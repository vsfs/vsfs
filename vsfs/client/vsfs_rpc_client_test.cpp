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
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/server_map.h"
#include "vsfs/index/index_info.h"
#include "vsfs/rpc/mock_rpc_clients.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::ContainerEq;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::Throw;
using ::testing::_;
using boost::shared_ptr;
using std::map;
using std::string;
using std::to_string;
using std::unique_ptr;
using vobla::Status;
using vsfs::index::IndexInfo;
using vsfs::rpc::MockIndexServerClient;
using vsfs::rpc::MockMasterServerClient;
using vsfs::rpc::TestRpcClientFactory;

namespace vsfs {
namespace client {

typedef VSFSClient::IndexUpdateRequest IndexUpdateRequest;

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
    uint64_t segment = std::numeric_limits<uint64_t>::max() / num_index_servers;
    for (int i = 0; i < num_index_servers; ++i) {
      RpcNodeAddress address;
      address.host = "localhost";
      address.port = 12000 + i;
      index_ring[i * segment] = address;
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
  EXPECT_CALL(*mock_master_, mkdir(_, _))
      .Times(1);
  EXPECT_CALL(*mock_master_, create("/foo/bar/.vsfs/index/0", _, _, _));
  EXPECT_CALL(*mock_master_, add_subfile(_, _))
      .Times(2);
  EXPECT_CALL(*mock_index_, create_index(_));

  RpcFileInfo dir_info;
  dir_info.mode = 0755 | S_IFDIR;
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/bar"))
      .WillOnce(SetArgReferee<0>(dir_info));

  EXPECT_TRUE(test_client_->create_index(
      "/foo/bar", "index", IndexInfo::BTREE, INT64, 0755, 100, 100).ok());
}

TEST_F(VsfsRpcClientTest, TestUpdateIndex) {
  init_client(2, 4);

  vector<IndexUpdateRequest> requests;
  int op = IndexUpdateRequest::INSERT;
  requests.emplace_back(op, "/foo/bar/test0", "red", "0");
  requests.emplace_back(op, "/foo/bar/zoo/test1", "red", "1");
  requests.emplace_back(op, "/foo/some/test/path/test2", "blue", "2");

  RpcFileInfo info;
  info.mode = 0666 | S_IFDIR;
  RpcInvalidOp ouch;
  ouch.what = -ENOENT;
  // Index "red" is on "/foo"
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/.vsfs/red"))
      .Times(2)
      .WillRepeatedly(SetArgReferee<0>(info));
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/bar/.vsfs/red"))
      .WillRepeatedly(Throw(ouch));
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/bar/zoo/.vsfs/red"))
      .WillOnce(Throw(ouch));
  // Index "blue" is on "/foo/some/test'
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/some/test/path/.vsfs/blue"))
      .WillOnce(Throw(ouch));
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/some/test/.vsfs/blue"))
      .WillOnce(SetArgReferee<0>(info));
  EXPECT_CALL(*mock_master_, find_objects(_, _))
      .WillOnce(SetArgReferee<0>(vector<ObjectId>({1, 2, 3})));

  EXPECT_CALL(*mock_index_, update(_)).Times(2);
  EXPECT_TRUE(test_client_->update(requests).ok());
}

TEST_F(VsfsRpcClientTest, TestGetParentPathToIndexPathMap) {
  typedef VSFSRpcClient::IndexUpdateTask IndexUpdateTask;
  IndexUpdateTask task(test_client_.get());

  vector<IndexUpdateRequest> requests;
  for (int i = 0; i < 100; i++) {
    requests.emplace_back(IndexUpdateRequest::UPDATE,
                          string("/foo/bar/test") + to_string(i),
                          "index", to_string(i));
  }
  for (const auto& request : requests) {
    task.add(&request);
  }
  init_client(2, 1);

  RpcFileInfo info;
  info.mode = 0666 | S_IFDIR;
  RpcInvalidOp ouch;
  ouch.what = -ENOENT;
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/bar/.vsfs/index"))
      .WillRepeatedly(Throw(ouch));
  EXPECT_CALL(*mock_master_, getattr(_, "/foo/.vsfs/index"))
      .WillOnce(SetArgReferee<0>(info));
  IndexUpdateTask::ParentPathToIndexPathMap index_map;
  EXPECT_TRUE(task.get_parent_path_to_index_path_map(&index_map).ok());

  IndexUpdateTask::ParentPathToIndexPathMap expected_map;
  expected_map["/foo/bar"]["index"] = "/foo/.vsfs/index";
  EXPECT_THAT(index_map, ContainerEq(expected_map));
}

TEST_F(VsfsRpcClientTest, TestReorderRequests) {
  init_client(1, 4);

  typedef VSFSRpcClient::IndexUpdateTask IndexUpdateTask;
  IndexUpdateTask task(test_client_.get());

  vector<IndexUpdateRequest> requests;
  requests.emplace_back(IndexUpdateRequest::INSERT,
                        "/foo/bar/abc/def/test0", "dog", "0");
  requests.emplace_back(IndexUpdateRequest::INSERT,
                        "/foo/bar/abc/def/test1", "dog", "1");
  requests.emplace_back(IndexUpdateRequest::INSERT,
                        "/foo/bar/bla/bla/test3", "cat", "2");
  for (const auto& req : requests) {
    task.add(&req);
  }

  IndexUpdateTask::ParentPathToIndexPathMap index_map;
  index_map["/foo/bar/abc/def"]["dog"] = "/foo/bar/.vsfs/dog";
  index_map["/foo/bar/bla/bla"]["cat"] = "/foo/bar/.vsfs/cat";

  IndexUpdateTask::ServerToRequestMap request_map;
  EXPECT_TRUE(task.reorder_requests_to_index_servers(index_map,
                                                     &request_map).ok());
  IndexUpdateTask::ServerToRequestMap expected_map;
  expected_map["localhost:12001"]["/foo/bar/.vsfs/dog/0"].push_back(0);
  expected_map["localhost:12001"]["/foo/bar/.vsfs/dog/0"].push_back(1);
  expected_map["localhost:12002"]["/foo/bar/.vsfs/cat/0"].push_back(2);
  EXPECT_THAT(request_map, ContainerEq(expected_map));
}

}  // namespace client
}  // namespace vsfs
