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
#include "vsfs/rpc/mock_rpc_clients.h"
#include "vsfs/client/vsfs_rpc_client.h"

using boost::shared_ptr;
using std::string;
using std::unique_ptr;
using vsfs::rpc::MockIndexServerClient;
using vsfs::rpc::MockMasterServerClient;
using vsfs::rpc::TestRpcClientFactory;

namespace vsfs {
namespace client {

class VsfsRpcClientTest : public ::testing::Test {
 protected:
  void SetUp() {
    master_factory_.reset(new TestRpcClientFactory<MockMasterServerClient>);
    index_factory_.reset(new TestRpcClientFactory<MockIndexServerClient>);
  }

  unique_ptr<VSFSRpcClient> test_client_;
  unique_ptr<TestRpcClientFactory<MockMasterServerClient>> master_factory_;
  unique_ptr<TestRpcClientFactory<MockIndexServerClient>> index_factory_;
};

TEST_F(VsfsRpcClientTest, TestInitialize) {
}

TEST_F(VsfsRpcClientTest, TestMkdir) {
}

}  // namespace client
}  // namespace vsfs
