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
#include <string>
#include "vsfs/api/cpp/vsfs_impl.h"

namespace vsfs {

using client::VSFSRpcClient;

VsfsImpl::VsfsImpl(const string& host, int port) : host_(host), port_(port) {
  conn_.reset(new VSFSRpcClient(host, port));
}

VsfsImpl::~VsfsImpl() {
}

Status VsfsImpl::connect() {
  return conn_->init();
}

}  // namespace vsfs
