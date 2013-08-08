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
  ThriftUtils::check_status(server_->create_index(index.root,
                                                  index.name,
                                                  index.index_type,
                                                  index.key_type));
}

void IndexServer::remove_index(const RpcIndexName &name) {
  // TODO(lxu): implement it later
  LOG(INFO) << "Removing index: "
            << name.name << " failed: function not implemented yet.";
}

void IndexServer::update(const RpcIndexUpdate &updates) {
  ThriftUtils::check_status(server_->update(updates));
}

void IndexServer::search(vector<int64_t>& results,  // NOLINT
                         const RpcComplexQuery& query) {
  ThriftUtils::check_status(server_->search(query, &results));
}

void IndexServer::info(RpcIndexInfo &info,  // NOLINT
                       const RpcIndexInfoRequest &request) {
  ThriftUtils::check_status(server_->info(request, &info));
}

}  // namespace indexd
}  // namespace vsfs
