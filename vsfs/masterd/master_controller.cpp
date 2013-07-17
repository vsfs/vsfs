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

#include <boost/filesystem.hpp>
#include <concurrency/PosixThreadFactory.h>
#include <concurrency/ThreadManager.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TNonblockingServer.h>
#include <server/TServer.h>
#include <string>
#include <utility>
#include <vector>
#include "vsfs/common/hash_util.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/index/index_info.h"
#include "vsfs/masterd/partition_manager.h"
#include "vsfs/masterd/index_namespace.h"
#include "vsfs/masterd/master_controller.h"
#include "vsfs/masterd/server_manager.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/masterd/master_server.h"

namespace fs = boost::filesystem;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::TProcessor;
using apache::thrift::protocol::TBinaryProtocolFactory;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::server::TNonblockingServer;
using apache::thrift::transport::TTransportFactory;
using std::string;
using vsfs::index::IndexInfo;

DEFINE_int32(port, 9876, "Sets the listening port.");
DEFINE_string(dir, ".", "Sets the directory to store metadata.");

namespace vsfs {
namespace masterd {

namespace {

string get_full_index_path(const string &root, const string &name) {
  return root + "/.vsfs/" + name;
}

}

MasterController::MasterController() : MasterController(FLAGS_dir) {
}

MasterController::MasterController(const string& basedir)
    : index_server_manager_(new ServerManager) {
  string abs_basedir = fs::absolute(basedir).string();
  index_partition_manager_.reset(new PartitionManager(
          abs_basedir + "/partition_map.db"));
  index_namespace_.reset(new IndexNamespace(
          abs_basedir + "/namespace.primer.db"));
}

MasterController::MasterController(IndexNamespaceInterface* idx_ns,
                                   PartitionManagerInterface* pm)
    : index_namespace_(idx_ns), index_partition_manager_(pm),
    index_server_manager_(new ServerManager) {
}

MasterController::~MasterController() {
}

Status MasterController::init() {
  auto status = index_namespace_->init();
  if (!status.ok()) {
    return status;
  }
  status = index_partition_manager_->init();
  return status;
}

void MasterController::start() {
  shared_ptr<MasterServer> handler(new MasterServer(this));
  shared_ptr<TProcessor> processor(new MasterServerProcessor(handler));
  shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());

  shared_ptr<ThreadManager> thread_manager =
      ThreadManager::newSimpleThreadManager(24);
  shared_ptr<PosixThreadFactory> thread_factory =
      shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  thread_manager->threadFactory(thread_factory);
  thread_manager->start();
  server_.reset(new TNonblockingServer(processor, protocol_factory,
                                       FLAGS_port, thread_manager));

  LOG(INFO) << "Master server is starting...";
  server_->serve();
  LOG(INFO) << "Master server quits...";
}

void MasterController::stop() {
  if (server_.get()) {
    LOG(INFO) << "Shutting master server down...";
    server_->stop();
  }
}

Status MasterController::join_index_server(const NodeInfo &node,
                                           RpcNodeAddressList *replicas) {
  CHECK_NOTNULL(replicas);
  LOG(INFO) << "IndexServer: " << node.address.host
            << ": " << node.address.port
            << " is trying to join the cluster.";
  Status status = index_server_manager_->add(node);
  if (!status.ok()) {
    LOG(ERROR) << "MasterController::join_index_server: "
               << status.message();
    return status;
  }

  const size_t kNumReplicaServers = 2;
  auto replica_servers = index_server_manager_->get_replica_servers(
      node, kNumReplicaServers);
  for (const auto& node : replica_servers) {
    replicas->emplace_back(node.address);
  }
  return status;
}

Status MasterController::create_index(const RpcIndexCreateRequest &request,
                                      RpcIndexLocation *index_location) {
  CHECK_NOTNULL(index_location);
  // TODO(eddyxu): move IndexNamespace into IndexPartitionManager, so they
  // share a single mutex to avoid inconsistency issues between them.
  Status status = index_namespace_->insert(request.root, request.name);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to create index on index namespace: "
               << status.message();
    return status;
  }
  string full_path = get_full_index_path(request.root, request.name);
  LOG(INFO) << "Creating index on partition 0: " << full_path;
  status = index_partition_manager_->add_index(full_path);
  if (!status.ok()) {
    // Roll back.
    LOG(ERROR) << "Failed to insert index (" << full_path
               << ") into index partition manager:" << status.message();
    index_namespace_->remove(request.root, request.name);
    return status;
  }

  NodeInfo node;
  // The first partition always starts from hash value 0.
  string partition_path =
      index_partition_manager_->get_partition_path(full_path, 0);
  size_t path_hash = HashUtil::file_path_to_hash(partition_path);
  status = index_server_manager_->get(path_hash, &node);
  if (!status.ok()) {
    // Roll back.
    LOG(ERROR) << "Failed to get index server for: " << full_path
               << ": " << status.message();
    index_namespace_->remove(request.root, request.name);
    index_partition_manager_->remove_index(full_path);
    return status;
  }
  index_location->full_index_path = partition_path;
  index_location->server_addr.host = node.address.host;
  index_location->server_addr.port = node.address.port;
  return Status::OK;
}

Status MasterController::locate_index(const RpcIndexLookupRequest &request,
                                      RpcIndexLocationList *results) {
  CHECK_NOTNULL(results);
  Status status;
  vector<string> index_paths;
  const string& name = request.name;  // index name.
  unordered_map<string, vector<uint64_t>> partition_to_file_id_map;
  for (const auto& dir_and_file_ids : request.dir_to_file_id_map) {
    const string& dir_path = dir_and_file_ids.first;
    VLOG(2) << "Lookup for directory: " << dir_path;
    string index_path;
    status = index_namespace_->find(dir_path, name, &index_path);
    if (!status.ok()) {
      LOG(ERROR) << "Can not locate index with name (" << name
                 << ") for directory: " << dir_path;
      return status;
    }

    string full_path = get_full_index_path(index_path, name);
    // A mapping from the patition path to a vector if positions of the
    // file ids in file id vector.
    unordered_map<string, vector<size_t>> partition_to_file_id_pos_map;
    const auto& file_id_vector = dir_and_file_ids.second;
    // TODO(lxu): use adaptive way to switch between copying the parition map
    // and directly quering the partition path.
    PartitionManager::PartitionMap index_partition_map;
    status = index_partition_manager_->copy_partition_map(
        full_path, &index_partition_map);
    for (auto file_id : file_id_vector) {
      string partition_path;
      index_partition_map.get(file_id, &partition_path);
      partition_to_file_id_map[partition_path].push_back(file_id);
    }
  }
  for (const auto &partition_and_file_ids : partition_to_file_id_map) {
    const string& partition_path = partition_and_file_ids.first;
    size_t path_hash = HashUtil::file_path_to_hash(partition_path);
    NodeInfo index_server;
    status = index_server_manager_->get(path_hash, &index_server);
    if (!status.ok()) {
      LOG(ERROR) << "Can not find index server for: " << partition_path;
      return status;
    }
    results->emplace_back();
    results->back().full_index_path = partition_path;
    results->back().server_addr = index_server.address;
    results->back().file_ids.reserve(partition_and_file_ids.second.size());
    for (auto file_id : partition_and_file_ids.second) {
      results->back().file_ids.push_back(file_id);
    }
  }

  return Status::OK;
}

}  // namespace masterd
}  // namespace vsfs
