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

#include <boost/shared_ptr.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/rpc_client_factory.h"

using boost::shared_ptr;
using std::string;
using std::unique_ptr;
using vsfs::rpc::RpcClient;
using vsfs::rpc::RpcClientFactoryInterface;

namespace vsfs {
namespace client {

// TODO(lxu): move this to /rpc/mock_rpc_client_factory.h as a common test
// framework.
template <typename MockClientType>
class TestRpcClientFactory : public RpcClientFactoryInterface<
    RpcClient<MockClientType>> {
 public:
  typedef RpcClient<MockClientType> ClientType;

  TestRpcClientFactory() {
    mock_client_.reset(new MockClientType);
    client_.reset(new ClientType(mock_client_));
  }

  shared_ptr<ClientType> open(const string& host, int port) {
    return client_;
  }

  /// Gets the underlying mock client object.
  MockClientType* mock_client() {
    return mock_client_.get();
  }

 private:
  shared_ptr<MockClientType> mock_client_;
  shared_ptr<ClientType> client_;
};


class VsfsRpcClientTest : public ::testing::Test {
 protected:
  void SetUp() {
  }

  unique_ptr<VSFSRpcClient> test_client_;
};

TEST_F(VsfsRpcClientTest, TestInitialize) {
}

TEST_F(VsfsRpcClientTest, TestMkdir) {
}

}  // namespace client
}  // namespace vsfs
