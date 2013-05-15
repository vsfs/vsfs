/*
 * Copyright 2012-2013 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_RPC_RPC_CLIENT_H_
#define VSFS_RPC_RPC_CLIENT_H_

#include <boost/shared_ptr.hpp>
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>
#include <string>
#include "vobla/macros.h"

namespace vsfs {
namespace rpc {

using apache::thrift::protocol::TBinaryProtocol;
using apache::thrift::protocol::TProtocol;
using apache::thrift::transport::TBufferedTransport;
using apache::thrift::transport::TSocket;
using apache::thrift::transport::TTransport;
using boost::shared_ptr;
using std::string;

/**
 * \template RpcClient
 * \brief A helper template of Thrift RPC client.
 *
 * Usage:
 * ~~~~~~~~~{.cpp}
 * RpcClient<FooServiceClient> foo_client("192.168.0.1", 8080);
 * foo_client.open();
 * foo_client->FooBar();
 * ~~~~~~~~~
 */
template <typename Client, typename Transport = TBufferedTransport>
class RpcClient {
 public:
  typedef Client HandleType;

  RpcClient(const string &addr, int port) : addr_(addr), port_(port) {
    socket_.reset(new TSocket(addr, port));
    transport_.reset(new Transport(socket_));
    protocol_.reset(new TBinaryProtocol(transport_));
    client_.reset(new Client(protocol_));
  }

  /**
   * \brief Directly construct a RpcClient with a handler.
   * \param client The client instance.
   * \note It should only be used for dependency injection.
   */
  explicit RpcClient(const shared_ptr<HandleType>& client) {
    client_ = client;
  }

  /// Destructs a RpcClient and reclaims the resources.
  ~RpcClient() {
    if (is_open()) {
      close();
    }
  }

  void open() {
    if (transport_) {
      transport_->open();
    }
  }

  void close() {
    if (transport_) {
      transport_->close();
    }
  }

  /// Returns true if the connection has already been established.
  bool is_open() const {
    if (transport_) {
      return transport_->isOpen();
    } else if (client_) {
      // This RpcClient is used for dependency injection.
      return true;
    }
    return false;
  }

  HandleType* handler() const {
    return client_.get();
  }

  HandleType* operator->() {
    return handler();
  }

  const HandleType* operator->() const {
    return handler();
  }

  const string& host() const {
    return addr_;
  }

  int port() const {
    return port_;
  }

 private:
  string addr_;
  int port_;
  shared_ptr<TTransport> socket_;
  shared_ptr<TTransport> transport_;
  shared_ptr<TProtocol> protocol_;
  shared_ptr<Client> client_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(RpcClient);
};

}  // namespace rpc
}  // namespace vsfs

#endif  // VSFS_RPC_RPC_CLIENT_H_
