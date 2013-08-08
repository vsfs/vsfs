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
namespace masterd {

MasterServer::MasterServer(MasterController *ctrl) : controller_(ctrl) {
}

MasterServer::~MasterServer() {
}

void MasterServer::join_index_server(RpcNodeAddressList& results,  // NOLINT
                                     const RpcNodeInfo& info) {
  ThriftUtils::check_status(controller_->join_index_server(info, &results));
}

void MasterServer::join_master_server(const RpcNodeInfo& info) {
  ThriftUtils::check_status(controller_->join_master_server(info));
}

void MasterServer::get_all_masters(RpcConsistentHashRing& ring) {  // NOLINT
  ThriftUtils::check_status(controller_->get_all_masters(&ring));
}

void MasterServer::get_all_index_servers(RpcConsistentHashRing& ring) {  // NOLINT
  ThriftUtils::check_status(controller_->get_all_index_servers(&ring));
}

void MasterServer::mkdir(const string& path, const RpcFileInfo& info) {
  ThriftUtils::check_status(controller_->mkdir(path, info.mode, info.uid,
                                              info.gid));
}

void MasterServer::rmdir(const string& path) {
  ThriftUtils::check_status(controller_->rmdir(path));
}

void MasterServer::add_subfile(const string& parent, const string& subfile) {
  ThriftUtils::check_status(controller_->add_subfile(parent, subfile));
}

void MasterServer::remove_subfile(const string& parent, const string& subfile) {
  ThriftUtils::check_status(controller_->remove_subfile(parent, subfile));
}

void MasterServer::readdir(RpcFileList& subfiles, const string& path) {  // NOLINT
  ThriftUtils::check_status(controller_->readdir(path, &subfiles));
}

RpcObjectId MasterServer::create(const string& path, const int64_t mode,
                                 const int64_t uid, const int64_t gid) {
  RpcObjectId tmp = 0;
  ThriftUtils::check_status(controller_->create(path, mode, uid, gid, &tmp));
  return tmp;
}

void MasterServer::remove(const string& path) {
  ThriftUtils::check_status(controller_->remove(path));
}

RpcObjectId MasterServer::object_id(const string& path) {
  RpcObjectId tmp;
  ThriftUtils::check_status(controller_->object_id(path, &tmp));
  return tmp;
}

void MasterServer::getattr(RpcFileInfo& info, const string& path) {  // NOLINT
  ThriftUtils::check_status(controller_->getattr(path, &info));
}

void MasterServer::find_files(RpcFileList& files,  // NOLINT
                              const RpcObjectList& objects) {
  ThriftUtils::check_status(controller_->find_files(objects, &files));
}

void MasterServer::create_index(RpcIndexLocation& location,  // NOLINT
                                const RpcIndexCreateRequest& request) {
  ThriftUtils::check_status(controller_->create_index(request, &location));
}

void MasterServer::remove_index(const string& root, const string& name) {
  ThriftUtils::check_status(controller_->remove_index(root, name));
}

void MasterServer::locate_index(RpcIndexLocationList& loc_list,  // NOLINT
                                const RpcIndexLookupRequest& request) {
  ThriftUtils::check_status(controller_->locate_index(request, &loc_list));
}

}  // namespace masterd
}  // namespace vsfs
