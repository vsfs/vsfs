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

#include "vobla/status.h"
#include "vsfs/rpc/thrift_utils.h"
#include "vsfs/rpc/vsfs_types.h"

using vobla::Status;

namespace vsfs {

RpcInvalidOp ThriftUtils::status_to_rpc_invalid_op(const Status &status) {
  RpcInvalidOp invalid;
  invalid.what = status.error();
  invalid.why = status.message();
  return invalid;
}

void ThriftUtils::check_status(const Status& status) {
  if (!status.ok()) {
    throw ThriftUtils::status_to_rpc_invalid_op(status);
  }
}

}  // namespace vsfs
