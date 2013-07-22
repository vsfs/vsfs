/**
 * \file index_server_handler.cpp
 *
 * \brief Implementation of the RPC handler for IndexServer.
 *
 * Copyright 2012 (c) Lei Xu <eddyxu@gmail.com>
 */


#include <glog/logging.h>
#include <vector>
#include "vobla/status.h"
#include "vsfs/indexd/index_controller.h"
#include "vsfs/indexd/index_manager.h"
#include "vsfs/indexd/index_server.h"
#include "vsfs/rpc/vsfs_types.h"
#include "vsfs/rpc/thrift_utils.h"

namespace vsfs {
namespace indexd {

IndexServer::IndexServer(IndexController* server) : server_(server) {
}

IndexServer::~IndexServer() {
}

void IndexServer::create_index(const RpcIndexCreateRequest &index) {
  LOG(INFO) << "Create index: "
            << " path: " << index.root
            << " name: " << index.name
            << " type: " << index.index_type
            << " key: " << index.key_type;
  Status status = server_->create_index(index.root,
                                        index.name,
                                        index.index_type,
                                        index.key_type);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

void IndexServer::remove_index(const RpcIndexName &name) {
  // TODO(lxu): implement it later
  LOG(INFO) << "Removing index: "
            << name.name << " failed: function not implemented yet.";
}

void IndexServer::update(const RpcIndexUpdate &updates) {
  Status status = server_->update(updates);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

void IndexServer::search(vector<int64_t>& results,  // NOLINT
                                const RpcComplexQuery& query) {
  Status status = server_->search(query, &results);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

void IndexServer::info(RpcIndexInfo &info,  // NOLINT
                              const RpcIndexInfoRequest &request) {
  Status status = server_->info(request, &info);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

/* void IndexServer::migrate(const RpcIndexMigrationData &data) {
  Status status = server_->migrate(data);
  if (!status.ok()) {
    RpcInvalidOp ouch;
    ThriftUtils::StatusToRpcInvalidOp(status, &ouch);
    throw ouch;
  }
}

void IndexServer::update_to_remote_log(const RpcIndexUpdate& updates) {
  Status status = server_->update_to_remote_log(updates);
  if (!status.ok()) {
    RpcInvalidOp ouch;
    ThriftUtils::StatusToRpcInvalidOp(status, &ouch);
    throw ouch;
  }
}

void IndexServer::join_taker_node_server() {
  Status status = server_->join_taker_node_server();
  if (!status.ok()) {
    RpcInvalidOp ouch;
    ThriftUtils::StatusToRpcInvalidOp(status, &ouch);
    throw ouch;
  }
}

void IndexServer::join_taker_node_index_partition(
    const RpcIndexInfo &idx_info, int64_t sep) {
  Status status = server_->join_taker_node_index_partition(idx_info, sep);
  if (!status.ok()) {
    RpcInvalidOp ouch;
    ThriftUtils::StatusToRpcInvalidOp(status, &ouch);
    throw ouch;
  }
} */

}  // namespace indexd
}  // namespace vsfs
