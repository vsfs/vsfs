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

/**
 * \file client_metadata_test.cpp
 * \brief Integration tests of the VsfsRpcClient to perform metadata ops to the
 * master cluster.
 */

#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>
#include <stack>
#include <string>
#include <thread>
#include <vector>
#include <set>
#include "vobla/traits.h"
#include "vobla/file.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/index/index_info.h"
#include "vsfs/testing/local_vsfs_cluster.h"

using ::testing::ContainerEq;
using std::set;
using std::stack;
using std::thread;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;
using vsfs::LocalVsfsCluster;
using vsfs::client::VSFSRpcClient;
using vsfs::index::IndexInfo;
namespace fs = boost::filesystem;

namespace vsfs {
namespace client {

class ClientMetadataTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
  }

  void TearDown() {
    cluster_.reset();
    tmpdir_.reset();
  }

  void start(int num_masters, int num_indices) {
    cluster_.reset(new LocalVsfsCluster(
            tmpdir_->path(), num_masters, num_indices));
    cluster_->start();

    client_.reset(new VSFSRpcClient(cluster_->host(0), cluster_->port(0)));
    EXPECT_TRUE(client_->init().ok());
  }

  /// Recursively creates all absent directories in 'path'.
  void create_directories(const string& path) {
    stack<string> parent_dirs;
    auto tmp = path;
    parent_dirs.push(tmp);
    while (tmp != "/") {
      tmp = fs::path(tmp).parent_path().string();
      parent_dirs.push(tmp);
    }
    while (!parent_dirs.empty()) {
      auto dir = parent_dirs.top();
      parent_dirs.pop();
      client_->mkdir(dir, 0755, 100, 100).ok();
    }
  }

  void create_index(const string& root, const string& name) {
    create_directories(root);
    EXPECT_TRUE(client_->create_index(root, name, IndexInfo::BTREE,
                                      INT32, 0755, 100, 100).ok());
  }

  unique_ptr<TemporaryDirectory> tmpdir_;
  unique_ptr<LocalVsfsCluster> cluster_;
  unique_ptr<VSFSRpcClient> client_;
};

TEST_F(ClientMetadataTest, TestMakeDirs) {
  start(4, 2);

  create_directories("/test");

  set<string> expected_files;
  for (int i = 0; i < 100; i++) {
    EXPECT_TRUE(client_->mkdir("/test/dir" + to_string(i),
                               0755, 100, 100).ok());
    expected_files.insert("dir" + to_string(i));
  }

  vector<string> files;
  EXPECT_TRUE(client_->readdir("/test", &files).ok());
  set<string> actual_files(files.begin(), files.end());
  EXPECT_THAT(actual_files, ContainerEq(expected_files));
}

TEST_F(ClientMetadataTest, TestCreateIndices) {
  start(4, 1);
  create_directories("/foo/bar/test");
  create_directories("/foo/bar/zoo");

  EXPECT_TRUE(client_->create_index("/foo/bar", "blue", IndexInfo::BTREE,
                                  INT32, 0755, 100, 100).ok());
  EXPECT_TRUE(client_->create_index("/foo/bar/zoo", "blue", IndexInfo::BTREE,
                                  INT32, 0755, 100, 100).ok());
  struct stat stbuf;
  EXPECT_TRUE(client_->getattr("/foo/bar/.vsfs/blue", &stbuf).ok());
  EXPECT_TRUE(client_->getattr("/foo/bar/zoo/.vsfs/blue", &stbuf).ok());
}

TEST_F(ClientMetadataTest, TestLocateIndicesForSearch) {
  start(2, 1);
  create_index("/foo/bar/test", "blue");
  create_index("/foo/bar/test", "red");
  create_index("/foo/bar/zoo", "blue");

  ComplexQuery query;
  query.parse("/foo/bar?blue>100");

  vector<string> expected_indices = { "/foo/bar/test/.vsfs/blue",
                                      "/foo/bar/zoo/.vsfs/blue" };
  vector<string> actual_indices;
  EXPECT_TRUE(client_->locate_index_for_search(query, &actual_indices).ok());
  EXPECT_THAT(actual_indices, ContainerEq(expected_indices));
}

}  // namespace client
}  // namespace vsfs
