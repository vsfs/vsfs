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
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/common/types.h"
#include "vsfs/rpc/vsfs_types.h"

using apache::thrift::transport::TTransportException;
using std::string;
using std::vector;
using vobla::Status;
namespace fs = boost::filesystem;

DEFINE_int32(vsfs_client_num_thread, 16, "Sets the number of thread one "
             "VSFS client can use.");
const int kNumBackOffs = 3;

namespace vsfs {

using rpc::RpcClientFactory;

namespace client {

VSFSRpcClient::VSFSRpcClient(const string &host, int port)
    : master_client_factory_(new RpcClientFactory<MasterClientType>),
      index_client_factory_(new RpcClientFactory<IndexClientType>),
      host_(host), port_(port),
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

Status VSFSRpcClient::sync_master_server_map() {
  VLOG(0) << "Caches master server Consistent Hashing Ring.";
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

Status VSFSRpcClient::sync_index_server_map() {
  VLOG(0) << "Caches index server Consistent Hashing Ring.";
  RpcConsistentHashRing ring;
  try {
    master_client_->handler()->get_all_index_servers(ring);
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to get_all_index_servers(): " << e.what();
    return Status(e.getType(), e.what());
  }
  for (const auto& sep_and_address : ring) {
    NodeInfo node_info;
    node_info.address = sep_and_address.second;
    auto sep = sep_and_address.first;
    index_server_map_.add(sep, node_info);
  }
  return Status::OK;
}

Status VSFSRpcClient::init() {
  VLOG(0) << "Initialize a VSFSRpcClient.";
  auto status = connect(host_, port_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to connect to primary master("
               << host_ << ":" << port_ << "), because: "
               << status.message();
    return status;
  }
  status = sync_master_server_map();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to synchronize local master server mapping with "
               << " the primary master server: " << host_ << ":" << port_
               << ", because: " << status.message();
    return status;
  }
  status = sync_index_server_map();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to synchronize local index server mapping with "
               << " the primary master server: " << host_ << ":" << port_
               << ", because: " << status.message();
    return status;
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
  auto hash = HashUtil::file_path_to_hash(path);
  (void) mode;
  NodeInfo file_node;
  auto status = master_map_.get(hash, &file_node);
  if (!status.ok()) {
    VLOG(0) << "Failed to find a master server to create file: " << path;
    return status;
  }
  // Number of retries to create file to solve the conflicts.
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
  status = add_subfile(path);
  if (!status.ok()) {
    // Roll back the created file record from `file_node`.
    try {
      auto client = master_client_factory_->open(file_node.address);
      client->handler()->remove(path);
      master_client_factory_->close(client);
    } catch (TTransportException e) {  // NOLINT
      return Status(e.getType(), e.what());
    }
  }
  return Status::OK;
}

Status VSFSRpcClient::open(const string& path, ObjectId* oid) {
  CHECK_NOTNULL(oid);
  auto hash = HashUtil::file_path_to_hash(path);
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    auto client = master_client_factory_->open(node.address);
    *oid = client->handler()->object_id(path);
    master_client_factory_->close(client);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }
  return Status::OK;
}

Status VSFSRpcClient::unlink(const string& path) {
  // First remove the subfile from its parent node.
  NodeInfo parent_node;
  auto parent = fs::path(path).parent_path().string();
  auto parent_hash = HashUtil::file_path_to_hash(parent);
  auto filename = fs::path(path).filename().string();
  auto status = master_map_.get(parent_hash, &parent_node);
  if (!status.ok()) {
    return status;
  }
  // Removes this file from its parent directory first.
  try {
    auto client = master_client_factory_->open(parent_node.address);
    client->handler()->remove_subfile(parent, filename);
    master_client_factory_->close(client);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }

  NodeInfo node;
  auto hash = HashUtil::file_path_to_hash(path);
  status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    auto client = master_client_factory_->open(node.address);
    client->handler()->remove(path);
    master_client_factory_->close(client);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }
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
    LOG(ERROR) << "Failed to run mkdir RPC to master node {"
               << node.address.host << ":" << node.address.port
               << "} : " << status.message();
    return status;
  }
  status = add_subfile(path);
  if (!status.ok()) {
    // Roll back.
    try {
      auto client = master_client_factory_->open(node.address);
      client->handler()->remove(path);
      master_client_factory_->close(client);
    } catch (TTransportException e) {  // NOLINT
      return Status(e.getType(), e.what());
    }
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

Status VSFSRpcClient::readdir(const string& dirpath, vector<string>* files) {  // NOLINT
  CHECK_NOTNULL(files);
  auto hash = HashUtil::file_path_to_hash(dirpath);
  NodeInfo node;
  CHECK(master_map_.get(hash, &node).ok());
  try {
    auto client = master_client_factory_->open(node.address);
    client->handler()->readdir(*files, dirpath);
    master_client_factory_->close(client);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }
  return Status::OK;
}

Status VSFSRpcClient::create_index(const string& index_path,
                                   const string& index_name,
                                   int index_type,
                                   int key_type) {
  RpcIndexCreateRequest create_request;
  create_request.root = index_path;
  create_request.name = index_name;
  create_request.index_type = index_type;
  create_request.__set_key_type(key_type);

  // Asks the primary masterd for the index location.
  RpcIndexLocation index_loc;
  try {
    master_client_->handler()->create_index(index_loc, create_request);
  } catch (RpcInvalidOp e) {  // NOLINT
    LOG(ERROR) << "Rpc Invalid Op: " << e.what << ":" << e.why;
    return Status(e.what, e.why);
  }

  create_request.root = index_loc.full_index_path;
  try {
    auto index_client = index_client_factory_->open(index_loc.server_addr);
    index_client->handler()->create_index(create_request);
    index_client_factory_->close(index_client);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }
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

Status VSFSRpcClient::add_subfile(const string& path) {
  if (path == "/") {
    return Status::OK;
  }
  auto parent = fs::path(path).parent_path().string();
  auto hash = HashUtil::file_path_to_hash(parent);
  auto filename = fs::path(path).filename().string();
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  CHECK(status.ok());  // master_map_ must exist at this point.
  int backoff = kNumBackOffs;
  while (backoff) {
    try {
      auto client = master_client_factory_->open(node.address);
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
  return status;
}

}  // namespace client
}  // namespace vsfs
