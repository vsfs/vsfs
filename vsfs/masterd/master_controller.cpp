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

#include <boost/asio/ip/host_name.hpp>
#include <boost/filesystem.hpp>
#include <concurrency/PosixThreadFactory.h>
#include <concurrency/ThreadManager.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TNonblockingServer.h>
#include <transport/TBufferTransports.h>
#include <server/TServer.h>
#include <chrono>
#include <string>
#include <utility>
#include <vector>
#include "vsfs/common/leveldb_store.h"
#include "vsfs/common/path_util.h"
#include "vsfs/common/server_map.h"
#include "vsfs/common/thread.h"
#include "vsfs/index/index_info.h"
#include "vsfs/masterd/index_namespace.h"
#include "vsfs/masterd/master_controller.h"
#include "vsfs/masterd/master_server.h"
#include "vsfs/masterd/namespace.h"
#include "vsfs/masterd/partition_manager.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/rpc/rpc_client.h"

namespace fs = boost::filesystem;
using apache::thrift::TProcessor;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::protocol::TBinaryProtocolFactory;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::server::TNonblockingServer;
using apache::thrift::transport::TFramedTransport;
using apache::thrift::transport::TTransportFactory;
using std::string;
using vsfs::index::IndexInfo;


namespace vsfs {
namespace masterd {

MasterController::MasterController(const string& basedir,
                                   const string& host,
                                   int port,
                                   bool primary,
                                   const string& primary_host,
                                   int primary_port)
    : port_(port), is_primary_node_(primary), primary_host_(primary_host),
      primary_port_(primary_port) {
  if (host.empty()) {
    host_ = boost::asio::ip::host_name();
  } else {
    host_ = host;
  }
  if (!is_primary_node_ && primary_host_.empty()) {
    primary_host_ = boost::asio::ip::host_name();
  }
  // TODO(eddyxu): merge the primary node code in all constructors.
  if (is_primary_node_) {
    index_server_manager_.reset(new ServerMap);
    master_server_manager_.reset(new ServerMap);
    NodeInfo self;
    self.address.host = host_;
    self.address.port = port_;
    master_server_manager_->add(self);
  }

  string abs_basedir = fs::absolute(basedir).string();
  index_partition_manager_.reset(new PartitionManager(
          abs_basedir + "/partition_map.db"));
  index_namespace_.reset(new IndexNamespace(
          abs_basedir + "/namespace.primer.db"));
  namespace_.reset(new Namespace(
          abs_basedir + "/namespace.db"));
}

MasterController::MasterController(IndexNamespaceInterface* idx_ns,
                                   PartitionManagerInterface* pm,
                                   const string& host,
                                   int port,
                                   bool primary
                                   )
    : host_(host), port_(port), is_primary_node_(primary),
      index_namespace_(idx_ns), index_partition_manager_(pm) {
  if (is_primary_node_) {
    index_server_manager_.reset(new ServerMap);
    master_server_manager_.reset(new ServerMap);
    NodeInfo self;
    self.address.host = boost::asio::ip::host_name();
    self.address.port = port_;
    master_server_manager_->add(self);
  }
}

MasterController::~MasterController() {
}

Status MasterController::init() {
  auto status = index_namespace_->init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize index DB:" << status.message();
    return status;
  }
  status = index_partition_manager_->init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize partition DB:" << status.message();
    return status;
  }
  status = namespace_->init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize namespace DB:" << status.message();
    return status;
  }
  LOG(INFO) << "MasterController has fully intialized.";
  return status;
}

void MasterController::background_task() {
  if (!is_primary_node_) {
    LOG(INFO) << "A secondary master node: " << host_ << ":" << port_
              << " joins the metadata cluster.";
    // Join the primary master node.

    rpc::RpcClient<MasterServerClient, TFramedTransport>
        primary_master(primary_host_, primary_port_);
    primary_master.open();
    NodeInfo node_info;
    node_info.address.host = host_;
    node_info.address.port = port_;
    primary_master.handler()->join_master_server(node_info);
    LOG(INFO) << "Master " << host_ << ":" << port_
              << " successfully join the primary master (" << primary_host_
              << ":" << primary_port_ << ").";
  }

  /*
  const int kSleepSeconds = 30;
  while (runtime_status_ == RuntimeStatus::RUNNING) {
    std::unique_lock<mutex> lock(background_mutex_);
    background_cv_.wait_for(lock, std::chrono::seconds(kSleepSeconds));
    LOG(INFO) << "Masterd Background Task...";
  }
  */
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
                                       port_, thread_manager));

  if (is_primary_node_) {
    LOG(INFO) << "Primary master server is starting...";
  } else {
    LOG(INFO) << "Master server is starting...";
  }
  background_thread_ = thread(&MasterController::background_task, this);
  server_->serve();
  thread_manager->stop();
  if (is_primary_node_) {
    LOG(INFO) << "Primary master server quits...";
  } else {
    LOG(INFO) << "Master server quits...";
  }
}

void MasterController::stop() {
  runtime_status_ = RuntimeStatus::STOPPED;
  if (background_thread_.joinable()) {
    background_cv_.notify_all();
    background_thread_.join();
  }
  if (server_.get()) {
    LOG(INFO) << "Shutting master server down...";
    server_->stop();
  }
}

Status MasterController::join_master_server(const NodeInfo& node) {
  if (!is_primary_node_) {
    return Status(-1, "Attempt to join a non-primary master node.");
  }
  LOG(INFO) << "MasterServer: " << node.address.host << ":" << node.address.port
            << " is trying to join the cluster.";
  auto status = master_server_manager_->add(node);
  if (!status.ok()) {
    LOG(ERROR) << "MasterController::join_master_server: " << status.message();
  }
  return status;
}

Status MasterController::join_index_server(const NodeInfo &node,
                                           RpcNodeAddressList *replicas) {
  CHECK_NOTNULL(replicas);
  LOG(INFO) << "IndexServer: " << node.address.host
            << ": " << node.address.port
            << " is trying to join the cluster.";
  Status status = index_server_manager_->add(node);
  if (!status.ok()) {
    LOG(ERROR) << "MasterController::join_index_server: " << status.message();
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

Status MasterController::get_all_masters(RpcConsistentHashRing* ring) {
  CHECK_NOTNULL(ring);
  if (!is_primary_node_) {
    return Status(-1, "This is not the primary node.");
  }
  auto ch_ring_map = master_server_manager_->get_ch_ring_as_map();
  for (const auto& sep_and_node : ch_ring_map) {
    ring->insert(std::make_pair(sep_and_node.first,
                                sep_and_node.second.address));
  }
  return Status::OK;
}

Status MasterController::get_all_index_servers(RpcConsistentHashRing* ring) {
  CHECK_NOTNULL(ring);
  if (!is_primary_node_) {
    return Status(-1, "This is not the primary node.");
  }
  auto ch_ring_map = index_server_manager_->get_ch_ring_as_map();
  for (const auto& sep_and_node : ch_ring_map) {
    ring->insert(std::make_pair(sep_and_node.first,
                                 sep_and_node.second.address));
  }
  return Status::OK;
}

namespace {

bool is_valid_path(const string& path) {
  return fs::path(path).is_absolute();
}

}

Status MasterController::mkdir(const string& path, mode_t mode,
                               uid_t uid, gid_t gid) {
  if (!is_valid_path(path)) {
    return Status(-1, "The path must be absolute path.");
  }
  return namespace_->mkdir(path, mode, uid, gid);
}

Status MasterController::rmdir(const string& path) {
  return namespace_->rmdir(path);
}

Status MasterController::add_subfile(const string& parent,
                                     const string& subfile) {
  if (!is_valid_path(parent)) {
    return Status(-1, "The path must be absolute path.");
  }
  return namespace_->add_subfile(parent, subfile);
}

Status MasterController::remove_subfile(const string& parent,
                                        const string& subfile) {
  return namespace_->remove_subfile(parent, subfile);
}

Status MasterController::readdir(const string& path,  // NOLINT
                                 vector<string>* subfiles) {
  if (!is_valid_path(path)) {
    return Status(-1, "The path must be absolute path.");
  }
  return namespace_->readdir(path, subfiles);
}

Status MasterController::create(const string &path, int mode, uid_t uid,
                                gid_t gid, ObjectId *oid) {
  if (!is_valid_path(path)) {
    return Status(-1, "The path must be absolute path.");
  }
  return namespace_->create(path, mode, uid, gid, oid);
}

Status MasterController::remove(const string& path) {
  // It does not need to check the validation of the 'path', because the
  // invalidate path does not exist in the namespace as well.
  return namespace_->remove(path);
}

Status MasterController::object_id(const string& path, ObjectId* oid) {
  return namespace_->object_id(path, oid);
}

Status MasterController::getattr(const string& path, RpcFileInfo *info) {
  // Same to "remove", we do not need to check the validataion of path.
  return namespace_->getattr(path, info);
}

Status MasterController::setattr(const string& path, const RpcFileInfo& info) {
  return namespace_->setattr(path, info);
}

void MasterController::find_objects(const vector<string>& files,
                                      vector<ObjectId>* objects) {
  namespace_->find_objects(files, objects);
}

Status MasterController::find_files(const vector<ObjectId>& objects,
                                    vector<string>* files) {
  return namespace_->find_files(objects, files);
}

Status MasterController::create_index(const RpcIndexCreateRequest &request) {
  // TODO(eddyxu): move IndexNamespace into IndexPartitionManager, so they
  // share a single mutex to avoid inconsistency issues between them.
  Status status = index_namespace_->insert(request.root, request.name);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to create index on index namespace: "
               << status.message();
    return status;
  }

  auto full_path = PathUtil::index_path(request.root, request.name);
  VLOG(0) << "Create index: " << full_path;
  status = namespace_->mkdir(full_path, request.mode, request.uid, request.gid);
  if (!status.ok()) {
    VLOG(0) << "Status: " << status.message();
    LOG(ERROR) << "Failed to create index directory: " << full_path
               << " " << status.message();
    index_namespace_->remove(request.root, request.name);
  }
  return status;
}

Status MasterController::remove_index(const string& root, const string& name) {
  VLOG(0) << "Remove index: " << root << ", name: " << name;
  auto status = index_namespace_->remove(root, name);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove index record from index namespace:"
               << status.message();
  }
  // Even if removing from index namespace fails, we will try our best-efforts
  // to remove any related records.
  string full_path = PathUtil::index_path(root, name);
  index_partition_manager_->remove_index(full_path);
  return status;
}

Status MasterController::locate_indices(
    const string& root, const vector<string>& names, vector<string>* indices) {
  CHECK_NOTNULL(indices);
  for (const auto& name : names) {
    auto index_roots = index_namespace_->collect(root, name);
    for (const auto& idx_root : index_roots) {
      // TODO(lxu): provide a central place to build index path.
      auto idx_path = (fs::path(idx_root) / ".vsfs" / name).string();
      indices->push_back(idx_path);
    }
  }
  return Status::OK;
}

vector<string> MasterController::locate_indices(const string& root,
                                                bool recursive) {
  return index_namespace_->get_indices(root, recursive);
}

string MasterController::host() const {
  return host_;
}

int MasterController::port() const {
  return port_;
}

}  // namespace masterd
}  // namespace vsfs
