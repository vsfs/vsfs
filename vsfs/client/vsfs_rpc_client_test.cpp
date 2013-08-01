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
#include "vobla/status.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/server_map.h"
#include "vsfs/rpc/mock_rpc_clients.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::SetArgReferee;
using ::testing::_;
using boost::shared_ptr;
using std::map;
using std::string;
using std::unique_ptr;
using vobla::Status;
using vsfs::rpc::MockIndexServerClient;
using vsfs::rpc::MockMasterServerClient;
using vsfs::rpc::TestRpcClientFactory;

namespace vsfs {
namespace client {

namespace {

ACTION_P(SetServerMap, n) {
  for (int i = 0; i < n; i++) {
    NodeInfo node;
    node.address.host = "localhost";
    node.address.port = 10000 + i;
    arg0.add(i * 10000, node);
  }
}

}   // anonymous namespace

/// Unit tests for VsfsRpcClient.
class VsfsRpcClientTest : public ::testing::Test {
  typedef TestRpcClientFactory<MockMasterServerClient, MasterServerClient>
      MasterClientFactory;
  typedef TestRpcClientFactory<MockIndexServerClient, IndexServerClient>
      IndexClientFactory;

 protected:
  void SetUp() {
    master_factory_.reset(new MasterClientFactory);
    mock_master_ = master_factory_->mock_client();
    index_factory_.reset(new IndexClientFactory);
    mock_index_ = index_factory_->mock_client();

    // Creates a VsfsRpcClient with mock factories.
    test_client_.reset(new VSFSRpcClient(master_factory_.release(),
                                         index_factory_.release()));
  }

  void init_client(int num_masters) {
    RpcConsistentHashRing filled_ring;
    for (int i = 0; i < num_masters; ++i) {
      RpcNodeAddress address;
      address.host = "localhost";
      address.port = 10000 + i;
      filled_ring[i * 10000] = address;
    }
    EXPECT_CALL(*mock_master_, get_all_masters(_))
        .WillOnce(SetArgReferee<0>(filled_ring));
    test_client_->init();
  }

  unique_ptr<VSFSRpcClient> test_client_;
  unique_ptr<MasterClientFactory> master_factory_;
  MockMasterServerClient* mock_master_;
  unique_ptr<IndexClientFactory> index_factory_;
  MockIndexServerClient* mock_index_;
};

TEST_F(VsfsRpcClientTest, TestInitialize) {
}

TEST_F(VsfsRpcClientTest, TestMkdir) {
  // VSFS client has not been initialized yet.
  EXPECT_FALSE(test_client_->mkdir("/abcd", 0x666, 100, 100).ok());

  init_client(2);
  EXPECT_CALL(*mock_master_, mkdir("/abcd", _));
  EXPECT_TRUE(test_client_->mkdir("/abcd", 0x666, 100, 100).ok());
}

}  // namespace client
}  // namespace vsfs
