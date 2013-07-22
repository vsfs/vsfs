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
#include <server/TThreadPoolServer.h>
#include <server/TThreadedServer.h>
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
using apache::thrift::server::TThreadedServer;
using apache::thrift::server::TThreadPoolServer;
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
DEFINE_string(index_controller_type, "nonblocking",
    "Sets the index controller type: threaded, threadpool or nonblocking");


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
    const string &basedir, const string &host, int port)
  : host_(host), port_(port), base_dir_(basedir),
    manager_(new IndexManager(basedir)),
    master_(new MasterClientType(FLAGS_master_addr, FLAGS_master_port)) {
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
    /* VLOG(2) << "Check for Migration on Index Server: "
            << host_ << " : " << port_; */
    // background_migration();
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

  if (FLAGS_index_controller_type == "threaded") {
    shared_ptr<TServerTransport> server_transport(
        new TServerSocket(port_));
    shared_ptr<TTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    server_.reset(new TThreadedServer(processor, server_transport,
                                      transport_factory, protocol_factory));
  } else if (FLAGS_index_controller_type == "nonblocking") {
    shared_ptr<ThreadManager> thread_manager =
        ThreadManager::newSimpleThreadManager(15);
    shared_ptr<PosixThreadFactory> thread_factory =
        shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
    thread_manager->threadFactory(thread_factory);
    thread_manager->start();
    server_.reset(new TNonblockingServer(processor, protocol_factory,
                                         port_, thread_manager));
  } else if (FLAGS_index_controller_type == "threadpool") {
    shared_ptr<TServerTransport> server_transport(
        new TServerSocket(port_));
    shared_ptr<TTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<ThreadManager> thread_manager =
        ThreadManager::newSimpleThreadManager(15);
    shared_ptr<PosixThreadFactory> thread_factory =
        shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
    thread_manager->threadFactory(thread_factory);
    thread_manager->start();
    server_.reset(new TThreadPoolServer(processor, server_transport,
                                        transport_factory, protocol_factory,
                                        thread_manager));
  } else {
    LOG(ERROR) << "Unregonized server type.";
    return;
    // TODO(lxu) Return an error;
  }

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
  /* if (forward_data_or_not()) {
    // filter out all updates that falls in range (split_pos, end) and
    // forward update requests to taker node in migration phase 1.
    filter_and_forward_migration_updates(updates);
  } */
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

/* Status IndexController::migrate(const RpcIndexMigrationData &data) {
  // Deserialize to index.
  Status status;
  status = deserialize_data_into_index(data);
  if (!status.ok()) {
    LOG(ERROR) << "Error Deserializing migrated index: "
               << status.message();
    return status;
  }
  return Status::OK;
}

Status IndexController::update_to_remote_log(const RpcIndexUpdate& updates) {
  return manager_->update(&const_cast<RpcIndexUpdate&>(updates));
}

Status IndexController::join_taker_node_server() {
  return join();
}

Status IndexController::join_taker_node_index_partition(
    const RpcIndexInfo& idx_info, int64_t sep) {
  CHECK_NOTNULL(master_.get());
  try {
    master_->handler()->add_index_partition(idx_info, sep);
  } catch(RpcInvalidOp ouch) {  // NOLINT
    LOG(ERROR) << "Failed to join the new index partition to master."
               << " Index Name: " << idx_info.name
               << " Index Path: " << idx_info.path
               << " at pos: " << sep;
    return Status(ouch.what, ouch.why);
  }
  LOG(INFO) << "Sucessfully added new Index Partition."
            << " Index Name: " << idx_info.name
            << " Index Path: " << idx_info.path
            << " at pos: " << sep;
  return Status::OK;
}

Status IndexController::filter_and_forward_migration_updates(
    const RpcIndexUpdate& updates) {
  // TODO(ziling): make it multi-threaded so that it will not be blocked.
  // TODO(ziling): add transaction id of the forward request.
  for (const auto& migration_record : migration_map_) {
    for (const auto& record_update_list : updates.updates) {
      if (migration_record.first == get_index_id(
              record_update_list.root_path, record_update_list.name)) {
        RpcIndexUpdate forward_update_request;
        RpcIndexRecordUpdateList forward_update_list;
        forward_update_list.root_path = record_update_list.root_path;
        forward_update_list.name = record_update_list.root_path;
        for (const auto& update_record : record_update_list.record_updates) {
          if (is_in_range(
                 update_record.value, migration_record.second->range)) {
            // send these updates to taker node's log.
            forward_update_list.record_updates.push_back(update_record);
          }
        }
        forward_update_request.updates.push_back(forward_update_list);
        // send to taker node's log
        try {
          get_migration_connection(
            record_update_list.root_path, record_update_list.name)
              ->handler()->update_to_remote_log(forward_update_request);
        } catch(RpcInvalidOp ouch) {
          LOG(ERROR) << "Failed to forward data.";
          return Status(ouch.what, ouch.why);
        }
      }
    }
  }
  return Status::OK;
}

Status IndexController::deserialize_data_into_index(
    const RpcIndexMigrationData &data) {
  Status status;
  // create a new index.
  status = manager_->create(0,
                            data.idx_info.path,
                            data.idx_info.name,
                            data.idx_info.type,
                            data.idx_info.key_type);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to create new index: "
               << " index name: " << data.idx_info.name
               << " index path: " << data.idx_info.path
               << " because: " << status.message();
    return status;
  }
  // deserialize raw data into the new index.
  RangeIndexInterface* new_idx = manager_->get_range_index(data.idx_info.path,
                                                           data.idx_info.name);
  if (!new_idx) {
    LOG(ERROR) << "Can't find index: " << data.idx_info.path;
    return Status(-ENOENT, "Can't find new index.");
  }
  status = new_idx->deserialize(data.raw_data);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to deserialize index: " << data.idx_info.path;
    return status;
  }
  return Status::OK;
}

void IndexController::background_migration() {
  vector<IndexInfo*> results;
  manager_->find_indices_for_splitting(FLAGS_partition_threshold, &results);
  if (results.size() > 0) {
    for (auto& item : results) {  // NOLINT
      migrate_index(*item);
    }
  }
}

Status IndexController::migrate_index(const index::IndexInfo &idx_info) {
  VLOG(0) << "Starting index migration on giver node: " << host_;
  // Forward write requests to taker node's log
  Status status;
  // Split the index and get data
  const string idx_path = idx_info.path();
  const string idx_name = idx_info.index_name();
  const string full_idx_path = idx_path + "/.vsfs/" + idx_name;
  const int idx_key_type = idx_info.key_type();
  // Giver Node address.
  RpcNodeAddress giver_node_addr;
  giver_node_addr.host = host_;
  giver_node_addr.port = port_;
  // TODO(ziling): add MIGRATION flag here to prevent log merging on taker node.
  unique_ptr<RangeIndexInterface> migration_idx(
      RangeIndexInterface::create_range_index(idx_key_type));
  CHECK_NOTNULL(migration_idx.get());
  std::pair<int64_t, int64_t> range;
  CHECK_NOTNULL(manager_->get_range_index(idx_path, idx_name));
  // merge log to index.
  status = merge_log_to_index(idx_path, idx_name);
  if (!status.ok()) {
    LOG(ERROR) << status.message();
    return status;
  }
  if (manager_->get_range_index(idx_path, idx_name)->size() == 0) {
    LOG(ERROR) << "Error: Spliting an Empty Index: "
               << "Index Path: " << idx_path
               << "Index Name: " << idx_name;
    return Status(-ENOENT, "Can't split an empty index.");
  }
  range = manager_->get_range_index(
      idx_path, idx_name)->split(migration_idx.get());
  int64_t split_pos = range.first;
  int64_t end_pos = range.second;
  const string new_partition_name = get_new_partition_name(idx_name, split_pos);
  RpcIndexMigrationData data;
  VLOG(1) << "Splited index... "
          << " Index Path: " << idx_path
          << " Index Name: " << idx_name
          << " Ranging from : " << split_pos << " to " << end_pos;
  // TODO(ziling): wrap it into a function.
  string new_idx_full_path = idx_path + "/.vsfs/" + idx_name;
  data.idx_info.path = idx_path;
  data.idx_info.name = idx_name;
  data.idx_info.type = idx_info.index_type();
  data.idx_info.key_type = idx_info.key_type();
  data.idx_info.num_records = migration_idx.get()->size();
  LOG(INFO) << "Serializing index... "
            << " Index Path: " << idx_path
            << " Index Name: " << idx_name
            << " Num of records to migrate: " << data.idx_info.num_records;
  // serialize index to string.
  status = migration_idx.get()->serialize(&data.raw_data);
  if (!status.ok()) {
    LOG(ERROR) << "Can not serialize index: " << new_partition_name;
    return status;
  }
  // Ask Master for Taker Node address.
  RpcNodeAddress taker_node_addr;
  CHECK_NOTNULL(master_.get());
  // TODO(ziling): error status handling here.
  try {
    master_->handler()->get_new_index_partition_addr(
        taker_node_addr, split_pos, data.idx_info, giver_node_addr);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    LOG(ERROR) << "Failed to get index server for new index partition.";
    return Status(ouch.what, ouch.why);
  }
  LOG(INFO) << "Successfully get taker node address for migration: "
            << taker_node_addr.host << " : " << taker_node_addr.port;

  // Make sure taker_node and giver_node is not the same.
  // if (giver_node_addr.host == taker_node_addr.host &&
  //    giver_node_addr.port == taker_node_addr.port) {
  //  LOG(ERROR) << "Error: Giver node and taker node are the same.";
  //  return Status(-1, "Cannot migrate to itself.");
  // }

  // Make connection to Taker node.
  NodeInfo taker_node_info;
  taker_node_info.address = taker_node_addr;
  add_migration_record(
      idx_path, new_partition_name, split_pos, end_pos, taker_node_info);
  IndexControllerClientType* taker_node =
      get_migration_connection(idx_path, new_partition_name);
  CHECK_NOTNULL(taker_node);
  taker_node->open();
  if (!taker_node->is_open()) {
    LOG(ERROR) << "Error: Connection not opened.";
    remove_migration_record(idx_path, new_partition_name);
    return Status(-1, "Error Connection.");
  }

  // Phase 1: Data Pre-copy
  // TODO(ziling): add MIGRATING flag to taker node's log to prevent log merge.
  // Migrate Data. This operation could taker very long time.
  // TODO(ziling): use multi-threading to accerlerate data copying.
  try {
    taker_node->handler()->migrate(data);
  } catch(RpcInvalidOp ouch) {  // NOLINT
    LOG(ERROR) << "Failed to migrate index to taker node: "
               << taker_node_addr.host << ":" << taker_node_addr.port;
    remove_migration_record(idx_path, new_partition_name);
    return Status(ouch.what, ouch.why);
  }
  LOG(INFO) << "Migrated Index from Giver: " << giver_node_addr.host
            << " To Taker: " << taker_node_addr.host;

  // Phase 2: Merge at taker node & Join ring.
  // First Taker Node join the ring and the requests will be transfer to
  // Taker node.
  // TODO(ziling): check if there is any consistency problem and corner case.
  // TODO(ziling): join the partition into index partition manager.
  if (!taker_node->is_open()) {
    LOG(ERROR) << "Error: Connection not opened.";
    remove_migration_record(idx_path, new_partition_name);
    return Status(-1, "Error Connection.");
  }
  // data.idx_info.path = full_idx_path;
  try {
    taker_node->handler()->join_taker_node_index_partition(
        data.idx_info, split_pos);
  } catch(RpcInvalidOp ouch) {  // NOLINT
    LOG(ERROR) << "Failed to let taker node: "
               << taker_node_addr.host << ":" << taker_node_addr.port
               << " join Master.";
    remove_migration_record(idx_path, new_partition_name);
    return Status(ouch.what, ouch.why);
  }
  // TODO(ziling): this flag only support 1 index migration. fix it.
  // TODO(ziling): should the forwarding end here?
  taker_node->close();
  // Any inconsistency?
  remove_migration_record(idx_path, new_partition_name);
  LOG(INFO) << "Taker Node: " << taker_node_addr.host
            << " has successfully joined the ring.";
  LOG(INFO) << "Migration of Index: path: " << idx_path
            << " name: " << idx_name
            << " from Giver Node: " << giver_node_addr.host
            << " to Taker Node: " << taker_node_addr.host
            << " has successfully completed!";
  // TODO(ziling): garbage collection on giver node.
  return Status::OK;
}

void IndexController::add_migration_record(
    const string &root_path, const string &name, int64_t split_pos,
    int64_t end_pos, const NodeInfo &node) {
  string idx_id = get_index_id(root_path, name);
  if (migration_map_.find(idx_id) != migration_map_.end()) {
    LOG(ERROR) << "Failed to add migration record: record already existed.";
    return;
  }
  unique_ptr<RangeIndexMigrationRecord> record(new RangeIndexMigrationRecord);
  record->range = std::make_pair(split_pos, end_pos);
  record->taker_node = node;
  record->connection.reset(new IndexControllerClientType(node.address.host,
                                                     node.address.port));
  migration_map_[idx_id] = std::move(record);
}

void IndexController::remove_migration_record(const string &root_path,
                                          const string &name) {
  string idx_id = get_index_id(root_path, name);
  if (migration_map_.find(idx_id) == migration_map_.end()) {
    LOG(ERROR) << "Failed to remove migration record: record did not exist.";
    return;
  }
  migration_map_.erase(idx_id);
}

IndexController::RangeIndexMigrationRecord* IndexController::get_migration_record(
    const string &root_path, const string &name) {
  string idx_id = get_index_id(root_path, name);
  if (migration_map_.find(idx_id) == migration_map_.end()) {
    LOG(ERROR) << "Failed to get migration record: record did not exist.";
    return nullptr;
  }
  return migration_map_[idx_id].get();
}

IndexController::IndexControllerClientType* IndexController::get_migration_connection(
    const string &root_path, const string &name) {
  string idx_id = get_index_id(root_path, name);
  if (migration_map_.find(idx_id) == migration_map_.end()) {
    LOG(ERROR) << "Failed to get migration record: record did not exist.";
    return nullptr;
  }
  return migration_map_[idx_id]->connection.get();
}

string IndexController::get_index_id(const string &root_path, const string &name) {
  return root_path + name;
}

string IndexController::get_new_partition_name(const string &old_name,
                                           int64_t split_pos) {
  unsigned pos = old_name.find_last_of(".");
  if (pos == string::npos) {
    return old_name + "." + lexical_cast<string>(split_pos);
  }
  string new_name = old_name.substr(0, pos)
      + "." + lexical_cast<string>(split_pos);
  return new_name;
}

bool IndexController::is_in_range(const string &key_str,
                              const FileIdRangeType &range) {
  int64_t key = lexical_cast<int64_t>(key_str);
  return key >= range.first && key <= range.second;
}

Status IndexController::merge_log_to_index(const string &root_path,
                                       const string &name) {
  if (!manager_->get_range_index(root_path, name)) {
    LOG(ERROR) << "Failed merging log on index: " << root_path << " " << name;
    return Status(-ENOENT, "Index does not exist.");
  }
  uint64_t txn_id = master_->handler()->get_txn_id();
  return manager_->merge_log_to_index(root_path, txn_id);
}

bool IndexController::forward_data_or_not() {
  return migration_map_.size() > 0 ? 1 : 0;
} */

}  // namespace indexd
}  // namespace vsfs
