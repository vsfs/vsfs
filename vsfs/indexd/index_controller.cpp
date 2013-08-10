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
#include <boost/shared_ptr.hpp>
#include <concurrency/PosixThreadFactory.h>
#include <concurrency/ThreadManager.h>
#include <event2/event.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TNonblockingServer.h>
#include <string.h>
#include <sys/time.h>
#include <transport/TServerSocket.h>
#include <transport/TTransportUtils.h>
#include <unistd.h>
#include <condition_variable>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "vsfs/index/index_info.h"
#include "vsfs/index/range_index.h"
#include "vsfs/indexd/index_controller.h"
#include "vsfs/indexd/index_manager.h"
#include "vsfs/indexd/index_server.h"

using apache::thrift::TProcessor;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::protocol::TBinaryProtocol;
using apache::thrift::protocol::TBinaryProtocolFactory;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::server::TNonblockingServer;
using apache::thrift::transport::TBufferedTransportFactory;
using apache::thrift::transport::TServerSocket;
using apache::thrift::transport::TServerTransport;
using apache::thrift::transport::TTransportFactory;
using boost::shared_ptr;
using std::vector;

DEFINE_string(host, "", "Manually sets the hostname of this IndexController. "
              "If unset, it will load the hostname from gethostname(3).");
DEFINE_int32(port, 5555, "Sets the listen port of IndexController.");
DEFINE_string(server_id, "", "Sets the server ID.");
DEFINE_int32(retry, 10, "Sets the number of retries to connect with "
             " replica machines.");
DEFINE_string(master_addr, "localhost", "Sets the address of the VSFS master "
              "node.");
DEFINE_int32(master_port, 9876, "Sets the port of the master node.");
// DEFINE_int32(partition_threshold, 500000, "Set the index partition
// threshold.");

namespace vsfs {

using index::RangeIndex;
using index::IndexInfo;

namespace indexd {

IndexController::IndexController(const string &basedir)
    : host_(FLAGS_host), port_(FLAGS_port), base_dir_(basedir),
      manager_(new IndexManager(basedir)),
      master_(new MasterClientType(FLAGS_master_addr, FLAGS_master_port)) {
}

IndexController::IndexController(
    const string& basedir, const string& host, int port,
    const string& master_host, int master_port)
  : host_(host), port_(port), base_dir_(basedir),
    manager_(new IndexManager(basedir)) {
  string mhost = master_host.empty() ? FLAGS_master_addr : master_host;
  int mport = master_port == 0 ? FLAGS_master_port : master_port;
    master_.reset(new MasterClientType(mhost, mport));
}

IndexController::IndexController(const string &basedir, const string &host,
                                 int port, MasterClientType *master)
  : host_(host), port_(port), base_dir_(basedir),
    manager_(new IndexManager(basedir)),
    master_(master) {
}

IndexController::~IndexController() {
  stop();
}

void IndexController::background_task() {
  const int kSleepSeconds = 30;
  LOG(INFO) << "Joining master: " << FLAGS_master_addr
            << ":" << FLAGS_master_port;
  join();

  while (status_ != STOPPED) {
    std::unique_lock<std::mutex> lock(background_mutex_);
    background_cv_.wait_for(lock, std::chrono::seconds(kSleepSeconds));
  }
}

void IndexController::start() {
  if (!master_) {
    // TODO(lxu): creates a connection to MasterNode.
  }

  status_ = RUNNING;
  shared_ptr<IndexServer> handler(new IndexServer(this));
  shared_ptr<TProcessor> processor(new IndexServerProcessor(handler));
  shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());

  shared_ptr<ThreadManager> thread_manager =
      ThreadManager::newSimpleThreadManager(15);
  shared_ptr<PosixThreadFactory> thread_factory =
      shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  thread_manager->threadFactory(thread_factory);
  thread_manager->start();
  server_.reset(new TNonblockingServer(processor, protocol_factory,
                                       port_, thread_manager));

  LOG(INFO) << "Starting IndexController at port:" << port_;
  backgroup_thread_ = thread(&IndexController::background_task, this);
  server_->serve();
  LOG(INFO) << "Shutting down completely.";
}

void IndexController::stop() {
  LOG(INFO) << "Attempts to shutdown...";
  status_ = STOPPED;
  if (backgroup_thread_.joinable()) {
    background_cv_.notify_all();
    backgroup_thread_.join();
  }
  if (server_) {
    server_->stop();
  }
}

Status IndexController::join() {
  CHECK_NOTNULL(master_.get());
  master_->open();
  NodeInfo node_info;

  if (host_.empty()) {
    node_info.address.host = boost::asio::ip::host_name();
  } else {
    node_info.address.host = host_;
  }
  node_info.address.port = port_;
  node_info.server_id = FLAGS_server_id;

  NodeAddressList replica_addresses;
  master_->handler()->join_index_server(replica_addresses, node_info);

  LOG(INFO) << "Successfully login to master and add myself to the ring.";
  LOG(INFO) << "My replica machines: ";
  for (const auto& node_addr : replica_addresses) {
    LOG(INFO) << "   " << node_addr.host << ":" << node_addr.port;
  }

  /* for (const auto& node_addr : replica_addresses) {
    unique_ptr<IndexControllerClientType> replica_machine(
        new IndexControllerClientType(node_addr.host, node_addr.port));

    int retry = FLAGS_retry;
    while (retry) {
      try {
        replica_machine->open();
      } catch (...) {  // NOLINT
        retry--;
        sleep(1);
        LOG(INFO) << "Retry " << retry << " more times...";
        continue;
      }
      break;
    }
    if (retry) {
      replica_machines_.emplace_back(replica_machine.release());
    } else {
      LOG(ERROR) << "Can not connect to the replica machine: "
                 << replica_machine->host() << ":" << replica_machine->port();
    }
  } */
  // Connects to replica servers.
  return Status::OK;
}

Status IndexController::create_index(
    const string &root_path, const string &name, int index_type,
    int key_type) {
  // TODO(eddyxu): construct a log entity and replica to other machines first.
  return manager_->create(0, root_path, name, index_type, key_type);
}

Status IndexController::update(const RpcIndexUpdate &updates) {
  // TODO(eddyxu): send logs to replica servers.
  return manager_->update(&const_cast<RpcIndexUpdate&>(updates));
}

Status IndexController::search(const RpcComplexQuery &query,
                           vector<int64_t>* results) {
  return manager_->search(query, results);
}

/* size_t IndexController::num_replica_machines() const {
  return replica_machines_.size();
} */

Status IndexController::info(const RpcIndexInfoRequest &request,
                         RpcIndexInfo *result) {
  return manager_->info(request, result);
}

}  // namespace indexd
}  // namespace vsfs
