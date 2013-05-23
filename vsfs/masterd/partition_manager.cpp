/*
 * Copyright 2011-2013 (c) Lei Xu <eddyxu@gmail.com>
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

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <glog/logging.h>
#include <string>
#include <utility>
#include <vector>
#include "vobla/map_util.h"
#include "vobla/status.h"
#include "vsfs/common/key_value_store.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/common/thread.h"
#include "vsfs/masterd/masterd.pb.h"
#include "vsfs/masterd/partition_manager.h"

namespace fs = boost::filesystem;
using std::string;
using std::to_string;
using std::vector;
using vobla::contain_key;

namespace vsfs {
namespace masterd {

PartitionManager::PartitionManager(const string &path)
    : file_path_(path), store_(new LevelDBStore(path)) {
}

PartitionManager::PartitionManager(LevelDBStore* store) : store_(store) {
}

namespace {

/// Deserialize a buffer to the PartitionMap.
Status string_to_partition_map(const string& buffer,
                               PartitionManager::PartitionMap *pm) {
  IndexPartition idx_partition;
  if (!idx_partition.ParseFromString(buffer)) {
    return Status(-1, "Failed to parse protobuf.");
  }
  for (int i = 0; i < idx_partition.partition_size(); ++i) {
    const auto& partition = idx_partition.partition(i);
    pm->insert(partition.hash_sep(), partition.path());
  }
  return Status::OK;
}

/// Serialize PartitionMap to a string buffer. If anything goes wrong, returns
/// an empty string.
string partition_map_to_string(const PartitionManager::PartitionMap& pm) {
  string proto_string;
  IndexPartition idx_partition;
  for (const auto& sep_and_path : pm) {
    auto partition = idx_partition.add_partition();
    partition->set_hash_sep(sep_and_path.first);
    partition->set_path(sep_and_path.second);
  }
  if (!idx_partition.SerializeToString(&proto_string)) {
    return "";
  }
  return std::move(proto_string);
}

}  // namespace

Status PartitionManager::init() {
  Status status = store_->open();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize PartitionManager: "
               << status.message();
    return status;
  }
  for (const auto& key_and_value : *store_) {
    const string& key = key_and_value.first;
    const string& buffer = key_and_value.second;
    PartitionMap new_partition_map;
    status = string_to_partition_map(buffer, &new_partition_map);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to load PartitionMap for " << key
                 << " : " << status.message();
      return status;
    }
    partition_map_by_index_path_[key].reset(new Partition);
    partition_map_by_index_path_[key]->partitions_.swap(new_partition_map);
  }
  return Status::OK;
}

Status PartitionManager::add_index(const string &path) {
  CHECK_NOTNULL(store_.get());
  MutexGuard lock(mutex_);
  if (contain_key(partition_map_by_index_path_, path)) {
    LOG(WARNING) << "The index is already existed:" << path;
    return Status::OK;
  }
  partition_map_by_index_path_[path].reset(new Partition);
  PartitionMap::key_type initial_key = 0;  // TODO(lxu): a random number?
  string node = path + "." + to_string(initial_key);
  partition_map_by_index_path_[path]->partitions_.insert(initial_key, node);

  return write_partition_map(path);
}

Status PartitionManager::remove_index(const string &path) {
  MutexGuard lock(mutex_);
  // TODO(lxu): it is needed to build a "unused consitent hash map list", and
  // move the removed PartitionMap to that list first to avoid the case
  // that erasing an in-used PartitionMap (i.e. refcount_ > 0).
  Status status = store_->remove(path);
  if (!status.ok()) {
    LOG(WARNING) << "Failed to remove the index from DB...abort!..."
                 << status.message();
    return status;
  }
  partition_map_by_index_path_.erase(path);
  return Status::OK;
}

Status PartitionManager::add_partition(const string &path,
                                       HashValueType sep) {
  Partition *partitions = nullptr;
  {
    MutexGuard lock(mutex_);
    auto iter = partition_map_by_index_path_.find(path);
    if (iter == partition_map_by_index_path_.end()) {
      LOG(ERROR) << "Attempt to insert a hash range to an non-existed index: "
                 << path << "  hash(" << sep << ")";
      return Status(-ENOENT, "The index does not existed.");
    }
    partitions = iter->second.get();
    partitions->refcount_++;
  }
  // Checks whether the `sep` has already existed.
  MutexGuard lock(partitions->mutex_);
  if (partitions->partitions_.has_key(sep)) {
    partitions->refcount_--;
    return Status(-1, "The separator is already existed.");
  }
  string node = path + "." + to_string(sep);
  partitions->partitions_.insert(sep, node);
  partitions->refcount_--;

  Status status = write_partition_map(path);
  if (!status.ok()) {
    // TODO(eddyxu): add recovery.
    LOG(ERROR) << "Failed to write partition map to DB when add partition for "
               << "(" << path << ", " << sep << "): " << status.message();
  }
  return status;
}

string PartitionManager::get_partition_path(const string &path,
                                            HashValueType hash) {
  Partition *partitions = nullptr;
  {
    MutexGuard lock(mutex_);
    if (!contain_key(partition_map_by_index_path_, path)) {
      LOG(ERROR) << "Attempt to find partition on non-existed index: "
                 << path;
      return "";
    }
    partitions = partition_map_by_index_path_[path].get();
    partitions->refcount_++;
  }
  MutexGuard lock(partitions->mutex_);
  string partition_path;
  partitions->partitions_.get(hash, &partition_path);
  partitions->refcount_--;
  return partition_path;
}

Status PartitionManager::get_all_partition_paths(
    const string &index_path, vector<string> *paths) {
  Partition *partitions = nullptr;
  {
    MutexGuard lock(mutex_);
    if (!contain_key(partition_map_by_index_path_, index_path)) {
      LOG(ERROR) << "Attempt to find partition on non-existed index: "
                 << index_path;
      return Status(-ENOENT, "The index does not existed.");
    }
    partitions = partition_map_by_index_path_[index_path].get();
    partitions->refcount_++;
  }
  MutexGuard lock(partitions->mutex_);
  partitions->partitions_.get_values(paths);
  partitions->refcount_--;
  return Status::OK;
}

Status PartitionManager::copy_partition_map(
    const string &full_index_path, PartitionMap *partition_map) {
  CHECK_NOTNULL(partition_map);
  Partition *partitions = nullptr;
  MutexGuard lock(mutex_);
  if (!contain_key(partition_map_by_index_path_, full_index_path)) {
    LOG(ERROR) << "Attempt to find partition on non-existed index: "
               << full_index_path;
    return Status(-ENOENT, "The index does not existed.");
  }
  partitions = partition_map_by_index_path_[full_index_path].get();
  *partition_map = partitions->partitions_;
  return Status::OK;
}

Status PartitionManager::write_partition_map(const string& path) {
  string buffer = partition_map_to_string(
      partition_map_by_index_path_[path]->partitions_);
  if (buffer.empty()) {
    LOG(WARNING) << "Failed to serialize PartitionMap for " << path;
    return Status(-1, "Failed to serialize PartitionMap");
  }
  return store_->put(path, buffer);
}

}   // namespace masterd
}   // namespace vsfs
