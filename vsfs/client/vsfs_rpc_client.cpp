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
  master_client_ = master_client_factory_->open(host, port);
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

Status VSFSRpcClient::create(const string &path, int64_t mode, int64_t uid,
                             int64_t gid, ObjectId* id) {
  if (!is_initialized()) {
    VLOG(1) << "The VSFS RPC client has not initialized yet.";
    return Status(-1, "The client has not initialized yet.");
  }
  auto parent = fs::path(path).parent_path().string();
  auto hash = HashUtil::file_path_to_hash(path);
  auto parent_hash = HashUtil::file_path_to_hash(parent);
  (void) mode;
  NodeInfo file_node;
  auto status = master_map_.get(hash, &file_node);
  if (!status.ok()) {
    VLOG(0) << "Failed to find a master server to create file: " << path;
    return status;
  }
  // Number of retries to create file to solve the conflicts.
  const int kNumBackOffs = 3;
  int backoff = kNumBackOffs;
  while (backoff) {
    try {
      auto client = master_client_factory_->open(file_node.address.host,
                                                 file_node.address.port);
      *id = client->handler()->create(path, mode, uid, gid);
      master_client_factory_->close(client);
      break;
    } catch (TTransportException e) {  // NOLINT
      // TODO(eddyxu): clear the conflict resolving algorithm later.
      status.set_error(e.getType());
      status.set_message(e.what());
      backoff--;
      continue;
    }
  }
  if (!status.ok()) {
    VLOG(0) << "Failed to create file: "
            << path << " because: " << status.message();
    return status;
  }
  NodeInfo parent_node;
  status = master_map_.get(parent_hash, &parent_node);
  CHECK(status.ok());  // master_map_ must exist at this point.
  backoff = kNumBackOffs;
  while (backoff) {
    try {
      auto client = master_client_factory_->open(parent_node.address.host,
                                                 parent_node.address.port);
      auto filename = fs::path(path).filename().string();
      client->handler()->add_subfile(parent, filename);
      master_client_factory_->close(client);
      break;
    } catch (TTransportException e) {  // NOLINT
      // TODO(eddyxu): clear the conflict resolving algorithm later.
      status.set_error(e.getType());
      status.set_message(e.what());
      backoff--;
      continue;
    }
  }
  if (!status.ok()) {
    // Roll back the created file record from `file_node`.
    try {
      auto client = master_client_factory_->open(file_node.address.host,
                                                 file_node.address.port);
      client->handler()->remove(path);
      master_client_factory_->close(client);
    } catch (TTransportException e) {  // NOLINT
      return Status(e.getType(), e.what());
    }
  }
  return Status::OK;
}

Status VSFSRpcClient::open(const string &path, int flag) {
  (void) path;
  (void) flag;
  return Status::OK;
}

Status VSFSRpcClient::unlink(const string& path) {
  (void) path;
  return Status::OK;
}

Status VSFSRpcClient::mkdir(
    const string& path, int64_t mode, int64_t uid, int64_t gid) {
  if (!is_initialized()) {
    VLOG(1) << "The VSFS RPC client has not initialized yet.";
    return Status(-1, "The client has not initialized yet.");
  }
  FilePathHashType hash = HashUtil::file_path_to_hash(path);
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    auto master_client = master_client_factory_->open(node.address.host,
                                                      node.address.port);
    RpcFileInfo dir_info;
    dir_info.mode = mode;
    dir_info.uid = uid;
    dir_info.gid = gid;
    master_client->handler()->mkdir(path, dir_info);
    master_client_factory_->close(master_client);
  } catch (TTransportException e) {  // NOLINT
    status = Status(e.getType(), e.what());
    LOG(ERROR) << "Failed to run mkdir RPC to master node: "
               << node.address.host << ":" << node.address.port
               << " because: " << status.message();
    return status;
  }
  return Status::OK;
}

Status VSFSRpcClient::rmdir(const string& path) {
  // TODO(eddyxu): refactory mkdir/rmdir and extract shared code.
  if (!is_initialized()) {
    VLOG(1) << "The VSFS RPC client has not initialized yet.";
    return Status(-1, "The client has not initialized yet.");
  }
  FilePathHashType hash = HashUtil::file_path_to_hash(path);
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    auto master_client = master_client_factory_->open(node.address.host,
                                                      node.address.port);
    master_client->handler()->rmdir(path);
    master_client_factory_->close(master_client);
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

Status VSFSRpcClient::info(const string& path,
                           vector<index::IndexInfo>* infos) {
  CHECK_NOTNULL(infos);
  (void) path;
  return Status::OK;
}

bool VSFSRpcClient::is_initialized() {
  return !master_map_.empty();
}

}  // namespace client
}  // namespace vsfs
