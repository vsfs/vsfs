/*
 * Copyright 2013 (c) Ziling Huang <hzlgis@gmail.com>
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

#include <glog/logging.h>
#include <string>
#include <vector>
#include "vsfs/common/thread.h"
#include "vobla/map_util.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/rpc/thrift_utils.h"
#include "vsfs/rpc/vsfs_types.h"
#include "vsfs/metad/meta_manager.h"

using std::to_string;
using std::vector;
using vobla::contain_key;
using vsfs::LevelDBStore;

namespace vsfs {
namespace metad {

MetaManager::MetaManager(const string &dbpath)
    : store_(new LevelDBStore(dbpath)) {
}

MetaManager::MetaManager(vsfs::LevelDBStore* store) : store_(store) {
}

MetaManager::~MetaManager() {
}

Status MetaManager::init() {
  Status status = store_->open();
  if (!status.ok()) {
    return status;
  }
  MutexGuard guard(lock_);
  for (const auto &key_and_value : *store_) {
    map_.insert(std::make_pair(std::stoi(key_and_value.first),
                               key_and_value.second));
  }
  return Status::OK;
}

Status MetaManager::insert(uint64_t file_id, const string &file_path) {
  MutexGuard guard(lock_);
  auto it = map_.find(file_id);
  if (it != map_.end()) {
    // if the file_id is already in the map, return error.
    VLOG(1) << "FileID [" << file_id << "] has already existed on FilePath: "
            << it->second;
    return Status(-EEXIST, "The FileID has existed.");
  } else {
    map_.insert(std::make_pair(file_id, file_path));
  }

  string key = to_string(file_id);
  auto status = store_->put(key, file_path);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to insert file ID into DB: key: "
               << key << " Reason: " << status.message();
    VLOG(0) << "Roll back the in-memory changes...";
    map_.erase(file_id);
  }
  return Status::OK;
}

Status MetaManager::insert(const RpcMetaDataList& metadata) {
  for (const RpcMetaData& file_id_and_path : metadata) {
    uint64_t key = static_cast<uint64_t>(file_id_and_path.file_id);
    auto status = insert(key, file_id_and_path.file_path);
    if (!status.ok()) {
      return status;
    }
  }
  return Status::OK;
}

Status MetaManager::remove(uint64_t file_id) {
  MutexGuard guard(lock_);
  auto it = map_.find(file_id);
  if (it == map_.end()) {
    return Status(-ENOENT, "The FileID does not exist.");
  }
  map_.erase(it->first);

  string key = to_string(file_id);
  auto status = store_->remove(key);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove file ID from DB: key: "
               << key << " Reason: " << status.message();
    VLOG(0) << "Roll back the in-memory changes...";
    map_.insert(std::make_pair(it->first, it->second));
  }
  return Status::OK;
}

Status MetaManager::find(uint64_t file_id, string *file_path) {
  CHECK_NOTNULL(file_path);
  MutexGuard guard(lock_);
  auto it = map_.find(file_id);
  if (it == map_.end()) {
    return Status(-ENOENT, "The FileID does not exist.");
  }
  *file_path = it->second;
  return Status::OK;
}

Status MetaManager::search(const vector<int64_t>& file_ids,
                           vector<string>* results) {
  CHECK_NOTNULL(results);
  // TODO(ziling): big lock here. Optimize it. Prevent any operation before
  // this search request finished.
  MutexGuard guard(lock_);
  for (auto id : file_ids) {
    uint64_t hash_id = static_cast<uint64_t>(id);
    // TODO(ziling): use thread pool to imrpve performance.
    auto it = map_.find(hash_id);
    if (it == map_.end()) {
      LOG(ERROR) << "Failed to find FileID: " << hash_id;
      return Status(-ENOENT, "The FileID does not exist.");
    }
    // TODO(ziling): handle duplicate records.
    results->push_back(it->second);
  }
  return Status::OK;
}

size_t MetaManager::size() {
  MutexGuard guard(lock_);
  return map_.size();
}

}  // namespace metad
}  // namespace vsfs
