/*
 * Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
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
#include "vobla/status.h"
#include "vobla/string_util.h"
#include "vsfs/api/cpp/vsfs.h"
#include "vsfs/api/cpp/vsfs_impl.h"

using std::string;
using vobla::Status;
using vobla::stringprintf;

namespace vsfs {

using api::VsfsImpl;

Vsfs::Vsfs(const string& host, int port) : impl_(new VsfsImpl(host, port)) {
}

Vsfs::Vsfs(VsfsImpl* vsfs_impl) : impl_(CHECK_NOTNULL(vsfs_impl)) {
}

Vsfs::~Vsfs() {
}

Status Vsfs::connect() {
  return impl_->connection()->init();
}

Status Vsfs::create(const string& path, int64_t mode, int64_t uid,
                    int64_t gid, ObjectId* id) {
  CHECK_NOTNULL(id);
  VLOG(0) << stringprintf("Creating file '%s' (mode=%ld, uid=%ld gid=%ld))",
                          path.c_str(), mode, uid, gid);
  return impl_->connection()->create(path, mode, uid, gid, id);
}

Status Vsfs::mkdir(const string& path, int64_t mode, int64_t uid, int64_t gid) {
  return impl_->connection()->mkdir(path, mode, uid, gid);
}

Status Vsfs::rmdir(const string& path) {
  return impl_->connection()->rmdir(path);
}

}  // namespace vsfs
