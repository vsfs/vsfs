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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <errno.h>
#include <glog/logging.h>
#include <sys/stat.h>
#include <cstring>
#include <string>
#include "vobla/clock.h"
#include "vobla/map_util.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/types.h"
#include "vsfs/masterd/namespace.h"
#include "vsfs/rpc/vsfs_types.h"
#include "vsfs/rpc/thrift_utils.h"

using vobla::Clock;
using vobla::contain_key;
using vobla::contain_key_and_value;
using vobla::find_or_null;
namespace fs = boost::filesystem;

namespace vsfs {
namespace masterd {

Namespace::Namespace(const string &metadb) : store_(new LevelDBStore(metadb)) {
}

Namespace::Namespace(LevelDBStore* store) : store_(store) {
}

Namespace::~Namespace() {
}

Status Namespace::init() {
  auto status = store_->open();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize namespace: can not init leveldb store."
               << status.message();
    return status;
  }
  MutexGuard guard(mutex_);
  // TODO(eddyxu): load all metadata from the leveldb.
  for (auto record : *store_) {
    // TODO(eddyxu): use leveldb prefix scan to eliminates the necessary of
    // if..else check.
    auto key = record.first;
    if (boost::starts_with(key, "/")) {  // metadata record.
      FileMetadata metadata;
      if (!ThriftUtils::deserialize(record.second, &metadata)) {
        return Status(-1, "The metadata db is corrupted.");
      }
      metadata_map_[key] = metadata;
      if (S_ISDIR(metadata.mode)) {
        directories_.emplace(DirectoryMap::value_type(key, Directory()));
      } else {
        id_to_path_map_[metadata.object_id] = key;
      }
    } else if (boost::starts_with(key, "dir:")) {  // directory mapping.
      string fullpath = key.substr(4);
      auto path = fs::path(fullpath);
      auto parent = path.parent_path().string();
      auto filename = path.filename().string();

      CHECK(directories_.count(parent));
      directories_[parent].subfiles.insert(filename);
    } else if (boost::starts_with(key, "meta:")) {  // system-wide metadata
    } else {
      return Status(-1, "The metadata db is corrupted.");
    }
  }
  return Status::OK;
}

bool Namespace::exists(const string &path) const {
  // Since it will only be called in distributed environment, the
  // inconsistency caused by the simultaneously updates is acceptable.
  // Therefore no lock is needed here.
  return contain_key(metadata_map_, path);
}

Status Namespace::file_path(ObjectId oid, string *path) {
  CHECK_NOTNULL(path);
  MutexGuard guard(mutex_);
  auto it = id_to_path_map_.find(oid);
  if (it == id_to_path_map_.end()) {
    return Status(-ENOENT, "The object ID does not exist.");
  }
  *path = it->second;
  return Status::OK;
}

Status Namespace::object_id(const string &path, ObjectId *oid) {
  CHECK_NOTNULL(oid);
  MutexGuard guard(mutex_);
  auto it = metadata_map_.find(path);
  if (it == metadata_map_.end()) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  *oid = it->second.object_id;
  return Status::OK;
}

Status Namespace::getattr(const string &path, RpcFileInfo *info) {
  CHECK_NOTNULL(info);
  MutexGuard guard(mutex_);
  auto meta = find_or_null(metadata_map_, path);
  if (!meta) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  info->id = meta->object_id;
  info->uid = meta->uid;
  info->gid = meta->gid;
  info->mode = meta->mode;
  info->size = meta->size;
  info->atime = meta->atime;
  info->ctime = meta->ctime;
  info->mtime = meta->mtime;
  return Status::OK;
}

Status Namespace::create(const string &path, int mode, uid_t uid,
                         gid_t gid, ObjectId *oid) {
  CHECK_NOTNULL(oid);
  MutexGuard guard(mutex_);
  if (contain_key(metadata_map_, path)) {
    return Status(-EEXIST, strerror(EEXIST));
  }
  auto& meta = metadata_map_[path];
  meta.mode = mode | S_IFREG;
  meta.gid = gid;
  meta.uid = uid;
  double now = Clock::real_clock()->now();
  meta.atime = now;
  meta.ctime = now;
  meta.mtime = now;
  meta.object_id = get_object_id(path);

  CHECK(!contain_key_and_value(id_to_path_map_, meta.object_id, path))
      << "The object ID and path pair has already existed: ("
      << meta.object_id << ", " << path << ")";
  id_to_path_map_[meta.object_id] = path;
  *oid = meta.object_id;

  auto status = store_metadata(path, meta);
  if (!status.ok()) {
    metadata_map_.erase(path);
    id_to_path_map_.erase(meta.object_id);
  }
  return status;
}

Status Namespace::remove(const string &path) {
  MutexGuard guard(mutex_);
  if (!contain_key(metadata_map_, path)) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  auto status = store_->remove(path);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove path from leveldb: " << status.message();
    return status;
  }
  auto& meta = metadata_map_[path];
  auto obj = meta.object_id;
  CHECK(contain_key_and_value(id_to_path_map_, obj, path))
      << "The object ID and path pair dose not exist: ("
      <<  obj << ", " << path << ")";
  metadata_map_.erase(path);
  id_to_path_map_.erase(obj);
  return Status::OK;
}

Status Namespace::mkdir(
    const string &path, mode_t mode, uid_t uid, gid_t gid) {
  MutexGuard guard(mutex_);
  // Master server does not check the existence of the parent directory. The
  // client need to check the responsibilty of the parent directory.
  if (contain_key(metadata_map_, path)) {
    return Status(-EEXIST, strerror(EEXIST));
  }
  auto& meta = metadata_map_[path];
  meta.mode = mode | S_IFDIR;
  meta.gid = gid;
  meta.uid = uid;
  double now = Clock::real_clock()->now();
  meta.atime = now;
  meta.ctime = now;
  meta.mtime = now;
  meta.object_id = 0;  // No object for directory.

  directories_.emplace(DirectoryMap::value_type(path, Directory()));

  auto status = store_metadata(path, meta);
  if (!status.ok()) {
    metadata_map_.erase(path);
    directories_.erase(path);
    return status;
  }
  return Status::OK;
}

Status Namespace::rmdir(const string &path) {
  MutexGuard guard(mutex_);
  auto dir = find_or_null(directories_, path);
  if (!dir) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  if (!dir->subfiles.empty()) {
    return Status(-ENOTEMPTY, strerror(ENOTEMPTY));
  }
  auto status = store_->remove(path);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to rmdir from leveldb: " << status.message();
    return status;
  }

  metadata_map_.erase(path);
  directories_.erase(path);
  return Status::OK;
}

namespace {

string directory_map_key(const string& path) {
  return string("dir:") + path;
}

}

Status Namespace::add_subfile(const string &parent, const string &subfile) {
  MutexGuard guard(mutex_);
  auto dir = find_or_null(directories_, parent);
  if (!dir) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  if (dir->subfiles.count(subfile) > 0) {
    return Status(-EEXIST, strerror(EEXIST));
  }
  auto fullpath = (fs::path(parent) / subfile).string();
  string value;  // just an empty payload.
  string key = directory_map_key(fullpath);
  auto status = store_->put(key, value);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to put subfile into persisent store: " << fullpath
               << " : " << status.message();
    return status;
  }
  dir->subfiles.insert(subfile);

  return Status::OK;
}

Status Namespace::remove_subfile(const string &parent, const string &subfile) {
  MutexGuard guard(mutex_);
  auto dir = find_or_null(directories_, parent);
  if (!dir) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  if (dir->subfiles.count(subfile) > 0) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  auto fullpath = (fs::path(parent) / subfile).string();
  auto key = directory_map_key(fullpath);
  auto status = store_->remove(key);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove subfile from persisent store: " << fullpath
               << " : " << status.message();
    return status;
  }
  dir->subfiles.erase(subfile);
  return Status::OK;
}

Status Namespace::readdir(const string &path, vector<string>* results) {  // NOLINT
  CHECK_NOTNULL(results);
  MutexGuard guard(mutex_);
  auto dir = find_or_null(directories_, path);
  if (!dir) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  results->assign(dir->subfiles.begin(), dir->subfiles.end());
  return Status::OK;
}

ObjectId Namespace::get_object_id(const string &path) {
  FilePathHashType hash_value = HashUtil::file_path_to_hash(path);

  // Object id is the first 16 bits from hash value and the last 48 bits from
  // the assigned obj id.
  ObjectId obj_id = (hash_value & 0xFF000000) + (next_obj_id_ & 0x00FFFFFF);
  next_obj_id_++;
  return obj_id;
}

Status Namespace::store_metadata(const string &path,
                                 const FileMetadata& metadata) {
  auto meta_buf = ThriftUtils::serialize(metadata);
  return store_->put(path, meta_buf);
}

}  // namespace masterd
}  // namespace vsfs
