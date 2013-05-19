/**
 * \file index_partition_manager.cpp
 *
 * \brief
 *
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
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
    : file_path_(path) {
}

PartitionManager::PartitionManager(KeyValueStore* store) : store_(store) {
}

Status PartitionManager::insert(const string &path) {
  MutexGuard lock(mutex_);
  if (contain_key(partition_map_by_index_path_, path)) {
    LOG(ERROR) << "The index is already existed:" << path;
    return Status(-EEXIST, "The index is already existed.");
  }
  partition_map_by_index_path_[path].reset(new Partition);
  PartitionMap::key_type initial_key = 0;  // TODO(lxu): a random number?
  string node = path + "." + to_string(initial_key);
  partition_map_by_index_path_[path]->partitions_.insert(initial_key, node);
  return Status::OK;
}

void PartitionManager::remove(const string &path) {
  MutexGuard lock(mutex_);
  // TODO(lxu): it is need to build a "unused consitent hash map list", and
  // move the removed PartitionMap to that list first to avoid the case
  // that erasing an in-used PartitionMap (i.e. refcount_ > 0).
  partition_map_by_index_path_.erase(path);
}

/*
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
  return Status::OK;
}
*/

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

string PartitionManager::partition_map_to_string(const PartitionMap& pm) {
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

// This function saves the partition_map_by_index_path_ into a berkeley DB,
// where the key is the index path, and the value is the serialized C.H ring.
// For each value, it starts from
//   - num_nodes
// Then it follows with the pair of (HashValueType, string).
//
// TODO(lxu):
//  - Looking for the solution to store it in Protobuf.
//  - Add a 'dirty' flag to 'Partition' to only flush dirty partitions.
Status PartitionManager::save() {
  Status status;
  /*
  const string tmp_db_path = file_path_ + ".tmp";
  {
    fs::remove(tmp_db_path);
    BDbIndex partition_db(tmp_db_path, BDbIndex::HASH);
    status = partition_db.open();
    if (!status.ok()) {
      VLOG(1) << "Failed to open db.";
      return status;
    }
    MutexGuard lock(mutex_);
    for (const auto& path_and_partition_map : partition_map_by_index_path_) {
      const auto& index_path = path_and_partition_map.first;
      const PartitionMap& map = path_and_partition_map.second->partitions_;
      stringstream ss(stringstream::out);
      boost::archive::binary_oarchive oa(ss);
      size_t num_nodes = map.num_nodes();
      oa << num_nodes;
      PartitionMap::const_iterator it = map.begin();
      for (size_t i = 0; i < num_nodes; ++i, ++it) {
        oa << it->first;
        oa << it->second;
      }
      string buf = ss.str();
      int ret = partition_db.put(index_path.data(), index_path.size() + 1,
                                 buf.data(), buf.size());
      if (ret) {
        VLOG(1) << "Failed to put " << index_path
                << " into partition map file.";
        return Status(ret, partition_db.error_message(ret));
      }
    }
    partition_db.close();
  }

  boost::system::error_code ec;
  if (fs::exists(file_path_)) {
    // Moves the old partition map to a backup file.
    const string backup_db = file_path_ + ".bak";
    fs::rename(file_path_, backup_db, ec);
    if (ec.value()) {
      return Status(ec.value(), ec.message());
    }
  }
  VLOG(1) << "Move " << tmp_db_path << " to " << file_path_;
  fs::rename(tmp_db_path, file_path_, ec);
  if (ec.value()) {
    return Status(ec.value(), ec.message());
  }
  */

  return Status::OK;
}

Status PartitionManager::load() {
  return Status::OK;
}

}   // namespace masterd
}   // namespace vsfs
