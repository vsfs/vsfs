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

#ifndef VSFS_RPC_RPC_CLIENT_FACTORY_H_
#define VSFS_RPC_RPC_CLIENT_FACTORY_H_

#include <boost/shared_ptr.hpp>
#include <transport/TBufferTransports.h>
#include <string>
#include "vsfs/rpc/rpc_client.h"

using apache::thrift::transport::TFramedTransport;
using std::string;

namespace vsfs {
namespace rpc {

/**
 * \class RpcClientFactoryInterface
 * \brief Basic class for the factory class of IndexServerClient.
 */
template <typename ClientType>
class RpcClientFactoryInterface {
 public:
  RpcClientFactoryInterface() = default;

  virtual ~RpcClientFactoryInterface() {}

  /**
   * \brief It creates a new RpcClient instance and pass the ownership of the
   * RpcClient to the caller. The caller must reclaim the ClientType.
   */
  virtual shared_ptr<ClientType> open(const string& host, int port) = 0;
};

/**
 * \class RpcClientFactory
 * \brief The factory class to create connected RpcClient.
 */
template <typename ClientType>
class RpcClientFactory : public RpcClientFactoryInterface<ClientType> {
 public:
  RpcClientFactory() = default;

  virtual ~RpcClientFactory() {}

  virtual boost::shared_ptr<ClientType> open(const string& host, int port) {
    boost::shared_ptr<ClientType> client(new ClientType(host, port));
    return client;
  }
};

}  // namespace rpc
}  // namespace vsfs

#endif  // VSFS_RPC_RPC_CLIENT_FACTORY_H_
