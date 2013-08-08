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
#include "vobla/traits.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/server_map.h"
#include "vsfs/index/index_info.h"
#include "vsfs/rpc/mock_rpc_clients.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::_;
using boost::shared_ptr;
using std::map;
using std::string;
using std::unique_ptr;
using vobla::Status;
using vsfs::index::IndexInfo;
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

  /**
   * \brief Initialize the client with appropriate cached server mapping.
   * \param num_master_server the number of master servers.
   * \param num_index_servers the number of index servers.
   */
  void init_client(int num_master_servers, int num_index_servers) {
    RpcConsistentHashRing master_ring, index_ring;
    for (int i = 0; i < num_master_servers; ++i) {
      RpcNodeAddress address;
      address.host = "localhost";
      address.port = 10000 + i;
      master_ring[i * 10000] = address;
    }
    for (int i = 0; i < num_index_servers; ++i) {
      RpcNodeAddress address;
      address.host = "localhost";
      address.port = 12000 + i;
      index_ring[i * 12000] = address;
    }
    EXPECT_CALL(*mock_master_, get_all_masters(_))
        .WillOnce(SetArgReferee<0>(master_ring));
    EXPECT_CALL(*mock_master_, get_all_index_servers(_))
        .WillOnce(SetArgReferee<0>(index_ring));
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
  EXPECT_FALSE(test_client_->mkdir("/abcd", 0666, 100, 100).ok());

  init_client(2, 1);
  EXPECT_CALL(*mock_master_, mkdir("/abcd", _));
  EXPECT_TRUE(test_client_->mkdir("/abcd", 0666, 100, 100).ok());
}

TEST_F(VsfsRpcClientTest, TestRmdir) {
  EXPECT_FALSE(test_client_->rmdir("/abcd").ok());

  init_client(2, 1);
  EXPECT_CALL(*mock_master_, rmdir("/abc"));
  EXPECT_TRUE(test_client_->rmdir("/abc").ok());
}

TEST_F(VsfsRpcClientTest, TestCreateSuccess) {
  init_client(2, 1);
  ObjectId oid;
  EXPECT_CALL(*mock_master_, create("/abc/def", 0666 | S_IFREG, 100, 100))
      .WillOnce(Return(1234));
  EXPECT_CALL(*mock_master_, add_subfile("/abc", "def"));

  EXPECT_TRUE(test_client_->create("/abc/def", 0666, 100, 100, &oid).ok());
  EXPECT_EQ(1234, oid);
}

TEST_F(VsfsRpcClientTest, TestCreateIndexSuccess) {
  init_client(2, 1);
  RpcIndexLocation loc;
  loc.full_index_path = "/foo/bar/.vsfs/index/0";
  EXPECT_CALL(*mock_master_, mkdir(_, _))
      .Times(3);
  EXPECT_CALL(*mock_master_, add_subfile(_, _))
      .Times(3);
  EXPECT_CALL(*mock_index_, create_index(_));

  RpcFileInfo dir_info;
  dir_info.mode = 0755 | S_IFDIR;
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/bar"))
      .WillOnce(SetArgReferee<0>(dir_info));

  EXPECT_TRUE(test_client_->create_index(
      "/foo/bar", "index", IndexInfo::BTREE, INT64, 0755, 100, 100).ok());
}

}  // namespace client
}  // namespace vsfs