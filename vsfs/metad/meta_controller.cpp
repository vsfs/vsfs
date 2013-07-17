/*
 * Copyright 2013 (c) Ziling Huang <hzlgis@gmail.com>
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

#include <boost/shared_ptr.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TThreadedServer.h>
#include <string.h>
#include <transport/TServerSocket.h>
#include <transport/TTransportUtils.h>
#include <unistd.h>
#include <condition_variable>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/vsfs_types.h"
#include "vsfs/metad/meta_server.h"
#include "vsfs/metad/meta_controller.h"

using apache::thrift::TProcessor;
using apache::thrift::protocol::TBinaryProtocol;
using apache::thrift::protocol::TBinaryProtocolFactory;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::transport::TBufferedTransportFactory;
using apache::thrift::transport::TServerSocket;
using apache::thrift::transport::TServerTransport;
using apache::thrift::transport::TTransportFactory;
using boost::shared_ptr;
using std::string;
using std::vector;
using vsfs::NodeAddressList;
using vsfs::NodeInfo;
using vsfs::rpc::RpcClient;

DEFINE_string(host, "", "Manually set the hostname of this MetaController.");
DEFINE_int32(port, 6666, "Set the listen port of MetaController.");
DEFINE_string(server_id, "", "Set the server ID.");
DEFINE_string(master_addr, "localhost", "Set the address of the master"
              "node.");
DEFINE_int32(master_port, 9876, "Set the port of the master node.");

namespace vsfs {
namespace metad {

MetaController::MetaController(const string &basedir)
    : host_(FLAGS_host), port_(FLAGS_port), base_dir_(basedir),
      manager_(new MetaManager(basedir)),
      master_(new MasterClientType(FLAGS_master_addr, FLAGS_master_port)) {
}

MetaController::~MetaController() {
  stop();
}

void MetaController::background_task() {
  LOG(INFO) << "Joining master: " << FLAGS_master_addr
            << ":" << FLAGS_master_port;
  join();
  // TODO(Ziling): background tasks
}

void MetaController::start() {
  shared_ptr<MetaServer> handler(new MetaServer(this));
  shared_ptr<TProcessor> processor(new MetaServerProcessor(handler));
  shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
  shared_ptr<TServerTransport> server_transport(
      new TServerSocket(port_));
  shared_ptr<TTransportFactory> transport_factory(
      new TBufferedTransportFactory());
  server_.reset(new TThreadedServer(processor, server_transport,
                                    transport_factory, protocol_factory));
  LOG(INFO) << "Starting MetaController at port:" << port_;
  background_thread_ = thread(&MetaController::background_task, this);
  server_->serve();
  LOG(INFO) << "Shutting down completely.";
}

void MetaController::stop() {
  LOG(INFO) << "Attempts to shutdown...";
  if (server_) {
    server_->stop();
  }
  if (background_thread_.joinable()) {
    background_cv_.notify_all();
    background_thread_.join();
  }
}

Status MetaController::join() {
  CHECK_NOTNULL(master_->handler());
  master_->open();
  NodeInfo node_info;

  if (host_.empty()) {
    char host_buffer[HOST_NAME_MAX];
    int ret = gethostname(host_buffer, HOST_NAME_MAX);
    if (ret) {
      return Status(ret, strerror(ret));
    }
    node_info.address.host = host_buffer;
  } else {
    node_info.address.host = host_;
  }
  node_info.address.port = port_;
  node_info.server_id = FLAGS_server_id;

  NodeAddressList replica_server_addresses;
  master_->handler()->join_meta_server(replica_server_addresses, node_info);

  // TODO(Ziling): join replica and LOG INFO
  return Status::OK;
}

Status MetaController::insert(uint64_t file_id, const string &file_path) {
  return manager_->insert(file_id, file_path);
}

Status MetaController::insert(const RpcMetaDataList& metadata) {
  return manager_->insert(metadata);
}

Status MetaController::remove(uint64_t file_id) {
  return manager_->remove(file_id);
}

Status MetaController::find(uint64_t file_id, string *file_path) {
  return manager_->find(file_id, file_path);
}

Status MetaController::find(const vector<int64_t>& file_ids,
                        vector<string>* results) {
  return manager_->find(file_ids, results);
}

}  // namespace metad
}  // namespace vsfs
