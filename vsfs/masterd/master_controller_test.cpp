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

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/index/index_info.h"
#include "vsfs/masterd/master_controller.h"
#include "vsfs/masterd/mock_index_namespace.h"
#include "vsfs/masterd/mock_partition_manager.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::Contains;
using ::testing::Key;
using ::testing::Pair;
using ::testing::Return;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::Status;
using vsfs::index::IndexInfo;


namespace vsfs {
namespace masterd {

static const int64_t kNumPartitions = 4;
static const int64_t kPartitonSize =
    std::numeric_limits<int64_t>::max() / kNumPartitions;

/// Unit tests for Master Server.
class MasterControllerTest : public ::testing::Test {
 protected:
  void SetUp() {
    index_namespace_ = new MockIndexNamespace;
    partition_manager_ = new MockPartitionManager;
    // controller_ manages to delete index_namespace and partition_manager.
    controller_.reset(new MasterController(index_namespace_,
                                           partition_manager_));
  }

  /// Evenly divide the C.H ring into 'num_index_servers' segements and let
  /// each index server manage one segment.
  void join_index_servers(int num_index_servers) {
    for (int i = 0; i < num_index_servers; ++i) {
      RpcNodeAddressList replicas;
      NodeInfo index_server;
      index_server.address.host = "192.168.1." + to_string(i);
      index_server.address.port = 5550 + i;
      ASSERT_TRUE(controller_->join_index_server(
          index_server, &replicas).ok());
    }
  }

  // Test Master Server instance.
  unique_ptr<MasterController> controller_;
  MockIndexNamespace* index_namespace_;
  MockPartitionManager* partition_manager_;
};

TEST_F(MasterControllerTest, TestCreateIndex) {
  join_index_servers(3);

  EXPECT_CALL(*index_namespace_, insert("/test/data", "name0"))
      .Times(1).WillOnce(Return(Status::OK));
  EXPECT_CALL(*partition_manager_, add_index("/test/data/.vsfs/name0"))
      .Times(1).WillOnce(Return(Status::OK));
  EXPECT_CALL(*partition_manager_,
              get_partition_path("/test/data/.vsfs/name0", 0))
      .Times(1).WillOnce(Return("test_path_0"));

  RpcIndexCreateRequest request;
  request.root = "/test/data";
  request.name = "name0";
  request.index_type = IndexInfo::BTREE;
  request.key_type = TypeIDs::INT16;

  RpcIndexLocation loc;
  EXPECT_TRUE(controller_->create_index(request, &loc).ok());
  EXPECT_EQ("test_path_0", loc.full_index_path);
}

}  // namespace masterd
}  // namespace vsfs
