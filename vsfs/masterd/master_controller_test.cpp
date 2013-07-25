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

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include "vobla/file.h"
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
using ::testing::_;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::Status;
using vsfs::index::IndexInfo;

DECLARE_bool(configsrv);

namespace vsfs {
namespace masterd {

static const int64_t kNumPartitions = 4;
static const int64_t kPartitonSize =
    std::numeric_limits<int64_t>::max() / kNumPartitions;

/// Unit tests for Master Server.
class MasterControllerTest : public ::testing::Test {
 protected:
  void SetUp() {
    FLAGS_configsrv = true;
    tmpdir_.reset(new vobla::TemporaryDirectory);
    // controller_ manages to delete index_namespace and partition_manager.
    controller_.reset(new MasterController(tmpdir_->path()));
    ASSERT_TRUE(controller_->init().ok());
  }

  void TearDown() {
    controller_.reset();  // make sure controller is destroyed before tmpdir.
    tmpdir_.reset();
  }

  void reset_mock_controller() {
    index_namespace_ = new MockIndexNamespace;
    partition_manager_ = new MockPartitionManager;
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

  /// A helper function to create file indices.
  Status create_index(const string &root_path, const string &name,
                      int index_type, int key_type,
                      RpcIndexLocation *loc) {
    CHECK_NOTNULL(loc);
    RpcIndexCreateRequest create_request;
    create_request.root = root_path;
    create_request.name = name;
    create_request.index_type = index_type;
    create_request.key_type = key_type;
    return controller_->create_index(create_request, loc);
  }

  Status create_index(const string &root_path, const string &name,
                      int index_type = IndexInfo::BTREE,
                      int key_type = UINT64) {
    RpcIndexLocation loc;
    return create_index(root_path, name, index_type, key_type, &loc);
  }

  unique_ptr<MasterController> controller_;
  MockIndexNamespace* index_namespace_;
  MockPartitionManager* partition_manager_;
  unique_ptr<vobla::TemporaryDirectory> tmpdir_;
};

TEST_F(MasterControllerTest, TestDirectoryOps) {
  EXPECT_FALSE(controller_->mkdir("abc-efg", 0x555, 100, 200).ok());
  EXPECT_TRUE(controller_->mkdir("/foo", 0x555, 100, 100).ok());

  EXPECT_FALSE(controller_->rmdir("test").ok());
  EXPECT_FALSE(controller_->rmdir("/").ok());
  EXPECT_FALSE(controller_->rmdir("/bar").ok());
  EXPECT_TRUE(controller_->rmdir("/foo").ok());
}

TEST_F(MasterControllerTest, TestReadDir) {
  vector<string> files;
  EXPECT_FALSE(controller_->readdir("/foo", &files).ok());

  controller_->mkdir("/foo", 0x755, 100, 100);
  controller_->mkdir("/foo/bar", 0x755, 100, 100);
  controller_->add_subfile("/foo", "bar");
  for (int i = 0; i < 20; i++) {
    controller_->add_subfile("/foo", "data" + to_string(i));
  }
  EXPECT_TRUE(controller_->readdir("/foo", &files).ok());
  EXPECT_EQ(21u, files.size());
}

TEST_F(MasterControllerTest, TestCreateIndex) {
  join_index_servers(3);

  RpcIndexCreateRequest request;
  request.root = "/test/data";
  request.name = "name0";
  request.index_type = IndexInfo::BTREE;
  request.key_type = TypeIDs::INT16;

  RpcIndexLocation loc;
  EXPECT_TRUE(controller_->create_index(request, &loc).ok());
  EXPECT_EQ("/test/data/.vsfs/name0.0", loc.full_index_path);
}

TEST_F(MasterControllerTest, TestLocateIndex) {
  join_index_servers(1);

  RpcIndexLocation index_loc;
  EXPECT_TRUE(create_index("/foo/bar", "energy", IndexInfo::BTREE, FLOAT,
                           &index_loc).ok());

  RpcIndexLookupRequest lookup_request;
  lookup_request.name = "energy";
  lookup_request.dir_to_file_id_map["/foo/bar/abc"].push_back(
      HashUtil::file_path_to_hash("/foo/bar/abc/efg"));
  lookup_request.dir_to_file_id_map["/foo/bar/bcd"].push_back(
      HashUtil::file_path_to_hash("/foo/bar/bcd/aaa"));
  RpcIndexLocationList locations;
  EXPECT_TRUE(controller_->locate_index(lookup_request, &locations).ok());
  EXPECT_EQ(1u, locations.size());
  EXPECT_EQ("/foo/bar/.vsfs/energy.0", locations[0].full_index_path);
  EXPECT_EQ(2u, locations[0].file_ids.size());
}

}  // namespace masterd
}  // namespace vsfs
