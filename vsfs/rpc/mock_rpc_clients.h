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

#ifndef VSFS_RPC_MOCK_RPC_CLIENTS_H_
#define VSFS_RPC_MOCK_RPC_CLIENTS_H_

#include <boost/shared_ptr.hpp>
#include <gmock/gmock.h>
#include <protocol/TProtocol.h>
#include <transport/TBufferTransports.h>
#include <string>
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/rpc_client_factory.h"

using boost::shared_ptr;
using apache::thrift::protocol::TProtocol;
using apache::thrift::transport::TFramedTransport;
using vsfs::rpc::RpcClient;
using vsfs::rpc::RpcClientFactoryInterface;

namespace vsfs {
namespace rpc {

class MockMasterServerClient : public MasterServerClient {
 public:
  /// Just makes TestRpcClientFactory happy to allocate this class.
  MockMasterServerClient() : MasterServerClient(shared_ptr<TProtocol>()) {
  }

  MOCK_METHOD1(join_master_server, void(const RpcNodeInfo&));
  MOCK_METHOD1(join_index_server, void(const RpcNodeInfo&));
  MOCK_METHOD1(get_all_masters, void(RpcConsistentHashRing&));
  MOCK_METHOD2(mkdir, void(const string&, const RpcFileInfo&));
  MOCK_METHOD1(rmdir, void(const string&));
  MOCK_METHOD4(create, RpcObjectId(const string&, const int64_t, const int64_t,
                                   const int64_t));
  MOCK_METHOD1(remove, void(const string&));
  MOCK_METHOD2(add_subfile, void(const string&, const string&));
};

/**
 * \brief A mock class for index server connection.
 */
class MockIndexServerClient : public IndexServerClient {
 public:
  /// Just makes TestRpcClientFactory happy to allocate this class.
  MockIndexServerClient() : IndexServerClient(shared_ptr<TProtocol>()) {
  }

  MOCK_METHOD1(update, void(const RpcIndexUpdate&));
};

template <typename MockClient, typename Client>
class TestRpcClientFactory : public RpcClientFactoryInterface<
    RpcClient<Client, TFramedTransport>> {
 public:
  typedef RpcClient<Client, TFramedTransport> ClientType;
  typedef MockClient MockClientType;

  TestRpcClientFactory() {
    mock_client_.reset(new MockClientType);
    client_.reset(new ClientType(mock_client_));
  }

  shared_ptr<ClientType> open(const string& host, int port) {
    (void) host;
    (void) port;
    return client_;
  }

  void close(shared_ptr<ClientType> client) {
    // Do nothing.
    (void) client;
  }

  /// Gets the underlying mock client object.
  MockClientType* mock_client() {
    return mock_client_.get();
  }

 private:
  shared_ptr<MockClientType> mock_client_;
  shared_ptr<ClientType> client_;
};

}  // namespace rpc
}  // namespace vsfs

#endif  // VSFS_RPC_MOCK_RPC_CLIENTS_H_
