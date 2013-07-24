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

#include <glog/logging.h>
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vsfs/index/index_info.h"
#include "vsfs/masterd/master_server.h"
#include "vsfs/masterd/master_controller.h"
#include "vsfs/rpc/thrift_utils.h"

using std::set;
using std::string;
using std::unique_ptr;
using std::vector;
using vobla::Status;

namespace vsfs {

using index::IndexInfo;

namespace masterd {

namespace {

void check_status(const Status& status) {
  if (!status.ok()) {
    throw ThriftUtils::StatusToRpcInvalidOp(status);
  }
}

}  // anonymous namespace

MasterServer::MasterServer(MasterController *ctrl) : controller_(ctrl) {
}

MasterServer::~MasterServer() {
}

void MasterServer::join_index_server(RpcNodeAddressList& results,  // NOLINT
                                     const RpcNodeInfo& info) {
  check_status(controller_->join_index_server(info, &results));
}

void MasterServer::mkdir(const string& path, const RpcFileInfo& info) {
  check_status(controller_->mkdir(path, info.mode, info.uid, info.gid));
}

void MasterServer::rmdir(const string& path) {
  check_status(controller_->rmdir(path));
}

void MasterServer::readdir(RpcFileList& subfiles, const string& path) {  // NOLINT
  check_status(controller_->readdir(path, &subfiles));
}

void MasterServer::create_index(RpcIndexLocation& location,  // NOLINT
                                const RpcIndexCreateRequest& request) {
  check_status(controller_->create_index(request, &location));
}

void MasterServer::locate_index(RpcIndexLocationList& loc_list,  // NOLINT
                                const RpcIndexLookupRequest& request) {
  check_status(controller_->locate_index(request, &loc_list));
}

}  // namespace masterd
}  // namespace vsfs
