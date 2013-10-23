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

#ifndef VSFS_API_CPP_VSFS_IMPL_H_
#define VSFS_API_CPP_VSFS_IMPL_H_

#include <boost/utility.hpp>
#include <memory>
#include <string>
#include "vobla/status.h"
#include "vsfs/client/vsfs_rpc_client.h"

using std::string;
using std::unique_ptr;
using vobla::Status;

namespace vsfs {

class VsfsImpl : boost::noncopyable {
 public:
  VsfsImpl(const string& host, int port);

  virtual ~VsfsImpl();

  /**
   * \brief Connect to the VSFS cluster.
   * \return Status::OK if success.
   */
  Status connect();

  Status disconnect();

  /**
   * \brief Creates an customize index on path, with index name.
   * \param path the root directory of
   */
  Status create(const string& path, const string& name);

 private:
  string host_;
  int port_;

  unique_ptr<client::VSFSRpcClient> conn_;
};

}  // namespace vsfs

#endif  // VSFS_API_CPP_VSFS_IMPL_H_
