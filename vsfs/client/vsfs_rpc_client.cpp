/*
 * Copyright 2012 (c) Lei Xu <eddyxu@gmail.com>
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

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/transport/TTransportException.h>
#include <string>
#include "vsfs/client/vsfs_rpc_client.h"

using apache::thrift::transport::TTransportException;
using std::string;
using vobla::Status;

DEFINE_int32(vsfs_client_num_thread, 16, "Sets the number of thread one "
             "VSFS client can use.");

namespace vsfs {
namespace client {

VSFSRpcClient::VSFSRpcClient(const string &host, int port)
    : host_(host), port_(port),
      thread_pool_(FLAGS_vsfs_client_num_thread) {
}

VSFSRpcClient::~VSFSRpcClient() {
  thread_pool_.close();
  thread_pool_.join();
  if (master_client_.get() != NULL) {
    disconnect();
  }
}

Status VSFSRpcClient::init() {
  VLOG(0) << "Initialize a VSFSRpcClient.";
  auto status = connect(host_, port_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to connect to primary master("
               << host_ << ":" << port_ << "), because: "
               << status.message();
  }
  // Gets a full map of all master servers.
  RpcConsistentHashRing ring;
  try {
    master_client_->handler()->get_all_masters(ring);
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to get_all_masters(): " << e.what();
    return Status(e.getType(), e.what());
  }
  for (const auto& sep_and_address : ring) {
    NodeInfo node_info;
    node_info.address = sep_and_address.second;
    auto sep = sep_and_address.first;
    master_map_.add(sep, node_info);
  }
  return Status::OK;
}

Status VSFSRpcClient::connect(const string &host, int port) {
  host_ = host;
  port_ = port;
  master_client_.reset(new MasterClientType(host_, port_));
  VLOG(1) << "Connecting to " << host_ << ":" << port;
  try {
    master_client_->open();
    VLOG(1) << "Successfully connected to masterd: " << host << ":" << port;
    return Status::OK;
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to connect to master node: "
        << host << ":" << port;
    return Status(e.getType(), e.what());
  } catch (...) {  // NOLINT
    LOG(ERROR) << "Unknown connection error. ";
    return Status(-1, "Unknown connection error");
  }
}

Status VSFSRpcClient::disconnect() {
  try {
    master_client_->close();
    return Status::OK;
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to disconnect from the master node: "
        << host_ << ":" << port_;
    return Status(e.getType(), e.what());
  }
}

}  // namespace client
}  // namespace vsfs
