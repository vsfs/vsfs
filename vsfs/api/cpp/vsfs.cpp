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
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/client/vsfs_client.h"

using std::string;
using vobla::Status;
using vobla::stringprintf;

// TODO(eddyxu): Merge with client/vsfs_rpc_client.h

namespace vsfs {

using client::VSFSClient;
using client::VSFSRpcClient;

Vsfs::Vsfs(const string& host, int port) :
    client_(new VSFSRpcClient(host, port)) {
}

Vsfs::Vsfs(VSFSClient* mock_client) : client_(CHECK_NOTNULL(mock_client)) {
}

Vsfs::~Vsfs() {
}

Status Vsfs::connect() {
  return client_->init();
}

Status Vsfs::create(const string& path, int64_t mode, int64_t uid,
                    int64_t gid, ObjectId* id) {
  CHECK_NOTNULL(id);
  VLOG(0) << stringprintf("Creating file '%s' (mode=%ld, uid=%ld gid=%ld))",
                          path.c_str(), mode, uid, gid);
  return client_->create(path, mode, uid, gid, id);
}

Status Vsfs::unlink(const std::string& path, ObjectId* id) {
  CHECK_NOTNULL(id);
  return client_->unlink(path, id);
}

Status Vsfs::getattr(const string& path, struct stat* stbuf) {
  CHECK_NOTNULL(stbuf);
  (void) path;
  // return client_->getattr(path, stbuf);
  return Status::OK;
}

Status Vsfs::mkdir(const string& path, int64_t mode, int64_t uid, int64_t gid) {
  return client_->mkdir(path, mode, uid, gid);
}

Status Vsfs::rmdir(const string& path) {
  return client_->rmdir(path);
}

}  // namespace vsfs
