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
#include "vsfs/rpc/vsfs_types.h"

using apache::thrift::transport::TFramedTransport;
using std::string;

namespace vsfs {
namespace rpc {

/**
 * \class RpcClientFactoryInterface
 * \brief The interface of the factory class for any RpcClient.
 *
 * The purpose of this factory class family is two-fold:
 *   - It offers the opportunity to optimize the allocation of costly RpcClient,
 *   e.g., to re-use alreay established connections if any.
 *   - It can also be used to create mock FooServerClient for the dependency
 *   injection purpose.
 */
template <typename Client>
class RpcClientFactoryInterface {
 public:
  // The ClientType is usually RpcClient<FooServerClient>.
  typedef Client ClientType;

  RpcClientFactoryInterface() = default;

  virtual ~RpcClientFactoryInterface() {}

  virtual boost::shared_ptr<ClientType> open(const RpcNodeAddress& addr) {
    return open(addr.host, addr.port);
  }

  /**
   * \brief It creates a new RpcClient instance and pass the ownership of the
   * RpcClient to the caller. The caller must reclaim the ClientType.
   */
  virtual boost::shared_ptr<ClientType> open(const string& host, int port) = 0;

  /**
   * \brief Close the connection, in concept.
   *
   * This function manages to reclaim the connection hold by the client.
   */
  virtual void close(boost::shared_ptr<ClientType> client) = 0;
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
    // Currently, it is just the simplest form: creating a new client for each
    // open() request. We can use this function to provide better resource
    // management for RpcClient later.
    boost::shared_ptr<ClientType> client(new ClientType(host, port));
    client->open();
    return client;
  }

  virtual void close(boost::shared_ptr<ClientType> client) {
    client->close();
  }
};

}  // namespace rpc
}  // namespace vsfs

#endif  // VSFS_RPC_RPC_CLIENT_FACTORY_H_
