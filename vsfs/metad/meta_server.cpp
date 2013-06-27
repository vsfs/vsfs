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

#include <glog/logging.h>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vsfs/rpc/thrift_utils.h"
#include "vsfs/rpc/vsfs_types.h"
#include "vsfs/metad/meta_server.h"
#include "vsfs/metad/meta_controller.h"

using std::string;
using std::vector;

namespace vsfs {
namespace metad {

MetaServer::MetaServer(MetaController *ctrl) : controller_(ctrl) {
}

MetaServer::~MetaServer() {
}

void MetaServer::insert(const RpcMetaDataList& metadata) {
  Status status = controller_->insert(metadata);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

void MetaServer::remove(int64_t file_id) {
  Status status = controller_->remove(file_id);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

void MetaServer::find(string& result, int64_t file_id) {  // NOLINT
  Status status = controller_->find(file_id, &result);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

void MetaServer::search(vector<string>& file_paths,  // NOLINT
                        const vector<int64_t>& file_ids) {
  Status status = controller_->find(file_ids, &file_paths);
  if (!status.ok()) {
    RpcInvalidOp ouch = ThriftUtils::StatusToRpcInvalidOp(status);
    throw ouch;
  }
}

}  // namespace metad
}  // namespace vsfs
