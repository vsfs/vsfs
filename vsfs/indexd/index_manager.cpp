/*
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <error.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <algorithm>
#include <chrono>  // NOLINT
#include <fstream>  // NOLINT
#include <functional>
#include <future>   // NOLINT
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "vobla/map_util.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/common/thread.h"
#include "vsfs/index/index_info.h"
#include "vsfs/index/range_index.h"
#include "vsfs/indexd/index_manager.h"
#include "vsfs/rpc/vsfs_types.h"

namespace fs = boost::filesystem;
using boost::lexical_cast;
using std::bind;
using std::future;
using std::packaged_task;
using std::string;
using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::contain_key;
using vsfs::HashUtil;

const int64_t kDefaultFlushLogSize = 8 * 1024 * 1024;  // 8MB.
const char kIndexFileExt[] = ".idx";
const char kLogFileExt[] = ".log";

DEFINE_bool(update_immediately, false,
            "Sets to true to immediately append index updates to indices.");
DEFINE_int64(flush_log_size, kDefaultFlushLogSize,
              "Sets the size to flush log in bytes. (default: 8*1024*1024).");

namespace vsfs {
namespace indexd {

using index::IndexInfo;
using index::RangeIndexInterface;

namespace {

/// Calculates the key for the index.
string get_index_key(const string &root_path, const string &name) {
  return root_path + ":" + name;
}

}  // anonymous namespace

IndexManager::RangeIndexWrapper::RangeIndexWrapper(
    const string &path, RangeIndexInterface* ri)
    : base_path_(path), index_(ri), status_(NORMAL), dirty_(false) {
  CHECK_NOTNULL(ri);
}

IndexManager::RangeIndexWrapper::RangeIndexWrapper(
    const string &path, RangeIndexInterface *ri, bool dty)
    : base_path_(path), index_(ri), status_(NORMAL), dirty_(dty) {
  CHECK_NOTNULL(ri);
}

IndexManager::RangeIndexWrapper::~RangeIndexWrapper() {
  // TODO(lxu): flush modified logs and image.
}

bool IndexManager::RangeIndexWrapper::dirty() const {
  return dirty_;
}

int IndexManager::RangeIndexWrapper::status() const {
  return status_;
}

void IndexManager::RangeIndexWrapper::set_status(int new_status) {
  MutexGuard lock(lock_);
  status_ = new_status;
}

RangeIndexInterface* IndexManager::RangeIndexWrapper::index() const {
  return index_.get();
}

Status IndexManager::RangeIndexWrapper::update(
    TxnIdType txn_id,
    RpcIndexRecordUpdateList *updates) {
  CHECK_NOTNULL(updates);
  Status status;
  if (FLAGS_update_immediately) {
    status = index_->apply(updates->record_updates);
    if (status.ok()) {
      MutexGuard lock(lock_);
      dirty_ = true;
    }
  } else {
    // Only appends updates to the log.
    unique_ptr<RpcIndexRecordUpdateList>
        new_update(new RpcIndexRecordUpdateList);
    swap(*new_update, *updates);
    MutexGuard lock(lock_);
    status = log_manager_.append(txn_id, new_update.release());
    if (!status.ok()) {
      LOG(ERROR) << "Failed to append log: " << status.message();
      return status;
    }
    dirty_ = true;
  }
  return status;
}

Status IndexManager::RangeIndexWrapper::merge(TxnIdType txn_id) {
  Status status;
  MutexGuard lock(lock_);
  if (status_ == MIGRATING) {
    LOG(ERROR) << "Can not merge a MIGRATING index.";
    return Status(-2, "This index is under migration.");
  }

  vector<const RpcIndexRecordUpdateList*> records;
  status = log_manager_.scan(txn_id, &records);
  if (!status.ok()) {
    return status;
  }
  VLOG(2) << "Found " << records.size() << " records to merge.";
  for (const auto updates : records) {
    status = index_->apply(updates->record_updates);
    if (!status.ok()) {
      return status;
    }
  }
  // Can not erase the merged log yet, because either the image (i.e. index)
  // and the logs have been flushed to disk yet.
  last_merged_log_id_ = txn_id;
  dirty_ = true;
  return Status::OK;
}

Status IndexManager::RangeIndexWrapper::search(
    TxnIdType txn_id, const RpcRangeQuery &query, vector<int64_t> *results) {
  CHECK_NOTNULL(results);
  Status status = merge(txn_id);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to merge index: " << query.index_path;
    return status;
  }
  index_->search(query, results);
  VLOG(1) << results->size() << " files are found.";
  if (!status.ok()) {
    LOG(ERROR) << "Failed to search on index: " << query.index_path;
    return status;
  }
  return Status::OK;
}

Status IndexManager::RangeIndexWrapper::flush_log(bool force) {
  string path = base_path_ + kLogFileExt;
  Status status;
  std::ofstream of(path, std::ofstream::out | std::ofstream::app);
  if (of.fail()) {
    LOG(ERROR) << "Failed to open file for flushing logs: " << path;
    return Status(-1, "Failed to flush logs.");
  }

  status = log_manager_.flush(FLAGS_flush_log_size, &of, force);
  of.close();
  if (status.ok()) {
    log_manager_.erase(std::min(last_merged_log_id_,
                                log_manager_.last_flushed_txn_id()));
  }
  return status;
}

Status IndexManager::RangeIndexWrapper::flush_index() {
  /*
  string path = base_path_ + kIndexFileExt;
  string buf;
  {
    MutexGuard lock(lock_);
    index_->serialize(&buf);
  }

  std::ofstream of(path, std::ofstream::out | std::ofstream::trunc);
  if (of.fail()) {
    LOG(ERROR) << "Failed to open range index file: " << path;
    return Status(-1, "Failed to open range index file.");
  }
  of << buf;
  of.close();
  // It does not need to keep the logs that has already being flushed into
  // the image.
  if (last_merged_log_id_) {
    log_manager_.erase(last_merged_log_id_);
  }
  */
  return Status::OK;
}

// ------ IndexManager -------------------
IndexManager::IndexManager(const string &basedir)
  : basedir_(basedir), running_(true) {
  bg_flusher_thread_ =
      thread(bind(&IndexManager::background_flush_thread, this));
}

IndexManager::~IndexManager() {
  flush();
  running_ = false;
  bg_flusher_cv_.notify_all();
  bg_flusher_thread_.join();
}

Status IndexManager::create(
    int64_t txn_id, const string &root_path, const string &name,
    int index_type, int key_type) {
  // Ignores transaction ID for now.
  (void) txn_id;

  if (root_path.empty() || name.empty()) {
    return Status(-1, "Invalid parameters.");
  }
  // const string index_key = get_index_key(root_path, name);
  // TODO(ziling): use root_path + name as key?
  const string index_key = root_path;
  MutexGuard lock(lock_);
  if (contain_key(index_info_map_, index_key)) {
    return Status(-EEXIST, "The index is existed.");
  }

  if (index_type == IndexInfo::BTREE) {
    unique_ptr<RangeIndexInterface> range_index(
        RangeIndexInterface::create_range_index(key_type));
    if (range_index.get() == nullptr) {
      LOG(ERROR) << "Failed to allocate RangeIndex: key_type:" << key_type;
      return Status(-EINVAL,
          string("Unknown key type: ") + lexical_cast<string>(key_type));
    }
    LOG(INFO) << "Inserting index key: " << index_key;
    index_info_map_.insert(
        make_pair(index_key, IndexInfo(root_path, name,
                                       index_type, key_type)));

    // Use hash value of root_path to calculate the index/log filename.
    string base_path = get_index_file_base_path(root_path);
    range_index_map_[index_key].reset(
        new RangeIndexWrapper(base_path, range_index.release(), true));
  } else {
    return Status(-EINVAL, "Unknown index type.");
  }
  return Status::OK;
}

Status IndexManager::update(RpcIndexUpdate *out_updates) {
  CHECK_NOTNULL(out_updates);
  RpcIndexUpdate updates;
  swap(updates, *out_updates);

  RangeIndexWrapper::TxnIdType txn_id = updates.txn_id;
  bool has_failure = false;
  Status status;
  if (FLAGS_update_immediately) {
    // Immediately to updates all indices.
    // TODO(lxu): use a thread-pool to perform updates on each index with
    // a separate thread.
    vector<future<Status>> futures;
    for (const auto& record_update_list : updates.updates) {
      packaged_task<Status(RangeIndexWrapper::TxnIdType,
                           const RpcIndexRecordUpdateList&)>
          pt(bind(&IndexManager::update_single_index, this,
                  std::placeholders::_1, std::placeholders::_2));
      futures.emplace_back(pt.get_future());
      thread(std::move(pt), txn_id, std::ref(record_update_list)).detach();
    }
    for (auto& fut : futures) {  // NOLINT
      fut.wait();
      if (!fut.get().ok()) {
        has_failure = true;
      }
    }
  } else {
    // Only appends the logs to each index and return.
    for (const auto& record_update_list : updates.updates) {
      status = update_single_index(txn_id, record_update_list);
      if (!status.ok()) {
        // has_failure = true;
        return status;
      }
    }
  }
  return has_failure ? Status(-1, "An error has occured") : Status::OK;
}

RangeIndexInterface* IndexManager::get_range_index(const string &index_path,
                                                   const string &name) {
  MutexGuard lock(lock_);
  auto iter = range_index_map_.find(index_path);
  if (iter == range_index_map_.end()) {
    return NULL;
  }
  return iter->second->index();
}

Status IndexManager::merge_log_to_index(const string &index_path,
                                        int64_t txn_id) {
  MutexGuard lock(lock_);
  auto iter = range_index_map_.find(index_path);
  if (iter == range_index_map_.end()) {
    LOG(ERROR) << "Merge on an non-existed index: " << index_path;
    return Status(-ENOENT, "The index does not exist.");
  }
  return iter->second->merge(txn_id);
}

Status IndexManager::update_single_index(
    RangeIndexWrapper::TxnIdType txn_id,
    const RpcIndexRecordUpdateList& updates) {
  const string index_key = updates.root_path;
  IndexInfo* info = get_index_info(index_key);
  if (!info) {
    LOG(ERROR) << "Update on an non-existed index: " << index_key;
    return Status(-ENOENT, "The index does not exist.");
  }
  Status status;
  if (info->index_type() == IndexInfo::BTREE) {
    RangeIndexWrapper* index = range_index_map_[index_key].get();
    CHECK_NOTNULL(index);
    VLOG(3) << "Updating BTREE index: " << index_key;
    status = index->update(txn_id,
                           &const_cast<RpcIndexRecordUpdateList&>(updates));
  }
  if (!status.ok()) {
    string message("Errors occurs when update index: ");
    message += index_key;
    LOG(ERROR) << message << ": reason: " << status.message();
    return Status(-1, message);
  }
  return Status::OK;
}

Status IndexManager::search(const RpcComplexQuery &query,
                            vector<int64_t>* results) {
  CHECK_NOTNULL(results);
  for (const auto& cq : query.range_queries) {
    // TODO(lxu): use thread pool to improve performance.
    IndexInfo* info = get_index_info(cq.index_path);
    if (!info) {
      LOG(ERROR) << "Searching on an non-existed index: " << cq.index_path;
      return Status(-ENOENT, "The index does not exist.");
    }
    if (info->index_type() == IndexInfo::BTREE) {
      RangeIndexWrapper* index =
          range_index_map_[cq.index_path].get();
      CHECK_NOTNULL(index);
      Status status;
      vector<int64_t> tmp;
      status = index->search(query.txn_id, cq, &tmp);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to search on index: " << cq.index_path;
        return status;
      }
      results->insert(results->end(), tmp.begin(), tmp.end());
    } else {
      LOG(ERROR) << "Uknown index type: " << info->index_type();
      return Status(-1, "Wrong index type.");
    }
  }
  return Status::OK;
}

Status IndexManager::info(const RpcIndexInfoRequest &request,
                          RpcIndexInfo *result) {
  // TODO(lxu): add multithread support.
  CHECK_NOTNULL(result);
  IndexInfo* info = get_index_info(request.path);
  if (!info) {
    LOG(ERROR) << "Failed to query the information on index: "
                << request.path;
    return Status(-ENOENT, "The index does not exist.");
  }
  result->type = info->index_type();
  result->key_type = info->key_type();
  VLOG(1) << "type: " << result->type << " key: " << result->key_type;
  if (info->index_type() == IndexInfo::BTREE) {
    RangeIndexWrapper* index_wrapper =
        range_index_map_[request.path].get();
    CHECK_NOTNULL(index_wrapper);
    index_wrapper->merge(request.txn_id);

    RangeIndexInterface* index = index_wrapper->index();
    CHECK_NOTNULL(index);
    result->num_records = index->size();
  } else {
    LOG(ERROR) << "Uknown index type: " << info->index_type();
    return Status(-1, "Wrong index type.");
  }
  return Status::OK;
}

Status IndexManager::flush() {
  if (basedir_.empty()) {
    // This function is called from the test cases.
    return Status::OK;
  }

  Status status;
  bool has_failure = false;
  // Flushes all RangeIndices.
  for (auto& ri_wrapper : range_index_map_) {  // NOLINT
    if (ri_wrapper.second->dirty()) {
      status = ri_wrapper.second->flush_index();
      if (!status.ok()) {
        LOG(ERROR) << "Failed to flush index: " << ri_wrapper.first
                   << " : " <<status.message();
        has_failure = true;
      }
    }
  }
  if (has_failure) {
    return Status(-1, "Errors occurred when flush indices.");
  }
  return Status::OK;
}

Status IndexManager::find_indices_for_splitting(const size_t threshold,
                                                vector<IndexInfo*>* results) {
  CHECK_NOTNULL(results);
  for (const auto& range_index : range_index_map_) {
    if (range_index.second->index()->size() >= threshold) {
      results->push_back(get_index_info(range_index.first));
    }
  }
  return Status::OK;
}

void IndexManager::background_flush_thread() {
  const int kFlushInterval = 30;  // flush interval in seconds.
  while (running_) {
    std::unique_lock<std::mutex> lock(bg_flusher_lock_);
    if (bg_flusher_cv_.wait_for(lock, std::chrono::seconds(kFlushInterval)) ==
        std::cv_status::timeout) {
    }
    // Scans all range_index_map_ to find dirty index.
    for (auto& path_and_index_wrapper : range_index_map_) {
      RangeIndexWrapper* wrapper = path_and_index_wrapper.second.get();
      if (wrapper->dirty()) {
        // TODO(lxu): when the log file is too large, flushing the index
        // and reclaim the log.
        wrapper->flush_log();
      }
    }
  }
}

IndexInfo* IndexManager::get_index_info(const string &index_info_key) {
  MutexGuard lock(lock_);
  return find_or_null(index_info_map_, index_info_key);
}

string IndexManager::get_index_file_base_path(const string &root_path) const {
  int64_t index_hash = HashUtil::file_path_to_hash(root_path);
  return basedir_ + "/" + lexical_cast<string>(index_hash);
}

}  // namespace indexd
}  // namespace vsfs
