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

#include <boost/filesystem.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/transport/TTransportException.h>
#include <string>
#include <vector>
#include "vsfs/common/types.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/client/vsfs_rpc_client.h"

using apache::thrift::transport::TTransportException;
using std::string;
using std::vector;
using vobla::Status;
namespace fs = boost::filesystem;

DEFINE_int32(vsfs_client_num_thread, 16, "Sets the number of thread one "
             "VSFS client can use.");

namespace vsfs {
namespace client {

namespace {

/// Returns true if the path is validated.
bool is_validate_path(const string& path) {
  fs::path tmp(path);
  return tmp.is_absolute();
}

}

VSFSRpcClient::VSFSRpcClient(const string &host, int port)
    : host_(host), port_(port),
      thread_pool_(FLAGS_vsfs_client_num_thread) {
}

VSFSRpcClient::VSFSRpcClient(MasterClientFactory* master_factory,
                             IndexClientFactory* index_factory)
    : master_client_factory_(master_factory),
      index_client_factory_(index_factory) {
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

Status VSFSRpcClient::create(const string &path, mode_t mode) {
  (void) path;
  (void) mode;
  return Status::OK;
}

Status VSFSRpcClient::open(const string &path, int flag) {
  (void) path;
  (void) flag;
  return Status::OK;
}

Status VSFSRpcClient::mkdir(
    const string& path, int64_t mode, int64_t uid, int64_t gid) {
  if (master_map_.empty()) {
    VLOG(1) << "The VSFS RPC client has not initialized yet.";
    return Status(-1, "The client has not initialized yet.");
  }
  if (!is_validate_path(path)) {
    VLOG(1) << "The directory " << path << " is not a validate path.";
    return Status(-1, "The directory path is not validate.");
  }
  FilePathHashType hash = HashUtil::file_path_to_hash(path);
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    // TODO(eddyxu): use client multiplex in the future.
    MasterClientType master_client(node.address);
    master_client.open();
    RpcFileInfo dir_info;
    dir_info.mode = mode;
    dir_info.uid = uid;
    dir_info.gid = gid;
    master_client.handler()->mkdir(path, dir_info);
    master_client.close();
  } catch (TTransportException e) {  // NOLINT
    status = Status(e.getType(), e.what());
    LOG(ERROR) << "Failed to run mkdir RPC to master node: "
               << node.address.host << ":" << node.address.port
               << " because: " << status.message();
    return status;
  }
  return Status::OK;
}

Status VSFSRpcClient::create_index(const string& index_path,
                                   const string& index_name,
                                   int index_type,
                                   int key_type) {
  (void) index_path;
  (void) index_name;
  (void) index_type;
  (void) key_type;
  return Status::OK;
}

Status VSFSRpcClient::search(const ComplexQuery& query,
                             vector<string>* results) {
  (void) query;
  CHECK_NOTNULL(results);
  return Status::OK;
}

Status VSFSRpcClient::update(const vector<IndexUpdateRequest>& updates) {
  (void) updates;
  return Status::OK;
}

Status VSFSRpcClient::import(const vector<string>& file_paths) {
  (void) file_paths;
  return Status::OK;
}

}  // namespace client
}  // namespace vsfs
