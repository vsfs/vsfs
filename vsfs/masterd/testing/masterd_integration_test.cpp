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

#define _GLIBCXX_USE_NANOSLEEP   // Fixed std::this_thread::sleep_for on centos.

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include "vobla/file.h"
#include "vsfs/rpc/vsfs_types.h"
#include "vsfs/masterd/master_controller.h"
#include "vsfs/masterd/testing/test_masterd_cluster.h"

using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;

namespace vsfs {
namespace masterd {

class MasterServerIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
  }

  void TearDown() {
    LOG(INFO) << "TearDown cluster.";
    /// Stops the cluster and wait all threads exit.
    test_cluster_.reset();
    tmpdir_.reset();
  }

  /**
   * \brief Starts a Master cluster with num_masters machines.
   * \pre num_masters >= 1
   * \note the first node is the primary one.
   */
  void start_cluster(int num_masters) {
    test_cluster_.reset(new TestMasterdCluster(tmpdir_->path(), num_masters));
    test_cluster_->start();
  }

  unique_ptr<vobla::TemporaryDirectory> tmpdir_;
  unique_ptr<TestMasterdCluster> test_cluster_;
};

TEST_F(MasterServerIntegrationTest, TestStartMasterServerCluster) {
  start_cluster(4);
  LOG(INFO) << "Fully started.";
  std::this_thread::sleep_for(std::chrono::seconds(2));
  RpcConsistentHashRing ring = test_cluster_->primary()->get_all_masters();
  EXPECT_EQ(4u, ring.size());
}

}  // namespace masterd
}  // namespace vsfs
