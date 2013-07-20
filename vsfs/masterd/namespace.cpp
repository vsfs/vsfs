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
#include <sys/stat.h>
#include <glog/logging.h>
#include <cstring>
#include <string>
#include "vobla/clock.h"
#include "vobla/map_util.h"
#include "vsfs/common/thread.h"
#include "vsfs/masterd/namespace.h"

using vobla::Clock;
using vobla::contain_key;

namespace vsfs {
namespace masterd {

Namespace::Namespace(const string &metadb) : store_(new LevelDBStore(metadb)) {
}

Namespace::Namespace(LevelDBStore* store) : store_(store) {
}

Namespace::~Namespace() {
}

Status Namespace::init() {
  return Status::OK;
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
  return Status::OK;
}

Status Namespace::readdir(const string &path, vector<string>* results) {  // NOLINT
  CHECK_NOTNULL(results);
  string prefix = path + "/";
  MutexGuard guard(mutex_);
  auto it = metadata_map_.lower_bound(path);
  if (it == metadata_map_.end() || it->first != path) {
    return Status(-ENOENT, strerror(ENOENT));
  }
  return Status::OK;
}

}  // namespace masterd
}  // namespace vsfs
