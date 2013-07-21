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

#include <errno.h>
#include <glog/logging.h>
#include <sys/stat.h>
#include <cstring>
#include <string>
#include "vobla/clock.h"
#include "vobla/map_util.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/types.h"
#include "vsfs/masterd/namespace.h"

using vobla::Clock;
using vobla::contain_key;
using vobla::contain_key_and_value;
using vobla::find_or_null;

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
  // TODO(eddyxu): load all metadata from the leveldb.
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

Status Namespace::file_id(const string &path, ObjectId *oid) {
  CHECK_NOTNULL(oid);
  MutexGuard guard(mutex_);
  auto it = metadata_map_.find(path);
  if (it == metadata_map_.end()) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  *oid = it->second.object_id;
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
  meta.mode = mode & S_IFREG;
  meta.gid = gid;
  meta.uid = uid;
  double now = Clock::real_clock()->now();
  meta.atime = now;
  meta.ctime = now;
  meta.mtime = now;

  // Obtain an new object id.
  FilePathHashType hash_value = HashUtil::file_path_to_hash(path);

  // Object id is the first 16 bits from hash value and the last 48 bits from
  // the assigned obj id.
  ObjectId obj_id = (hash_value & 0xFF000000) + (next_obj_id_ & 0x00FFFFFF);
  meta.object_id = obj_id;
  next_obj_id_++;
  CHECK(!contain_key_and_value(id_to_path_map_, obj_id, path))
      << "The object ID and path pair has already existed: ("
      <<  obj_id << ", " << path << ")";
  id_to_path_map_[obj_id] = path;
  *oid = obj_id;
  return Status::OK;
}

Status Namespace::remove(const string &path) {
  MutexGuard guard(mutex_);
  if (!contain_key(metadata_map_, path)) {
    return Status(-ENOENT, strerror(ENOENT));
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
  meta.mode = mode & S_IFDIR;
  meta.gid = gid;
  meta.uid = uid;
  double now = Clock::real_clock()->now();
  meta.atime = now;
  meta.ctime = now;
  meta.mtime = now;

  directories_.emplace(DirectoryMap::value_type(path, Directory()));
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
  metadata_map_.erase(path);
  directories_.erase(path);
  return Status::OK;
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
  dir->subfiles.insert(subfile);
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

}  // namespace masterd
}  // namespace vsfs
