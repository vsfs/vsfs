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

MasterController::MasterController()
    : index_server_manager_(new ServerManager) {

  FLAGS_dir = fs::absolute(FLAGS_dir).string();
  index_partition_manager_.reset(new PartitionManager(
          FLAGS_dir + "/partition_map.db"));
  index_namespace_.reset(new IndexNamespace(
          FLAGS_dir + "/namespace.primer.db"));
}

MasterController::~MasterController() {
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

}  // namespace masterd
}  // namespace vsfs
