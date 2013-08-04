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
 * \brief Integration tests of the client to issues metadata ops to the master
 * cluster.
 */

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include "vobla/file.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/masterd/testing/local_masterd_cluster.h"

using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;
using vsfs::client::VSFSRpcClient;
using vsfs::masterd::LocalMasterdCluster;

namespace vsfs {

class ClientMetadataTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
  }

  void TearDown() {
    cluster_.reset();
    tmpdir_.reset();
  }

  void start(int num_masters) {
    cluster_.reset(new LocalMasterdCluster(tmpdir_->path(), num_masters));
    cluster_->start();
  }

  unique_ptr<TemporaryDirectory> tmpdir_;
  unique_ptr<LocalMasterdCluster> cluster_;
};

TEST_F(ClientMetadataTest, TestMakeDirs) {
  start(4);

  VSFSRpcClient client(cluster_->host(0), cluster_->port(0));
  EXPECT_TRUE(client.init().ok());
  EXPECT_TRUE(client.mkdir("/", 0x666, 100, 100).ok());
}

}  // namespace vsfs
