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
#include <string>
#include "vobla/map_util.h"
#include "vsfs/common/thread.h"
#include "vsfs/masterd/namespace.h"

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
    return Status(-ENOENT, "The object ID does not existed.");
  }
  return Status::OK;
}

}  // namespace masterd
}  // namespace vsfs
