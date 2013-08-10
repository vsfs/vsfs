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
#include "vsfs/common/complex_query.h"
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

RpcComplexQuery ThriftUtils::complex_query_to_rpc_complex_query(
    const ComplexQuery& query) {
  RpcComplexQuery result;
  result.root = query.root();

  auto range_names = query.get_names_of_range_queries();
  for (const auto& name : range_names) {
    auto range = query.range_query(name);
    result.range_queries.emplace_back();
    auto& rpc_query = result.range_queries.back();
    rpc_query.name = name;
    rpc_query.lower = range->lower;
    rpc_query.lower_open = !range->lower_closed;
    rpc_query.upper = range->upper;
    rpc_query.upper_open = !range->upper_closed;
  }
  return result;
}

}  // namespace vsfs
