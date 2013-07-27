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
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include "vobla/file.h"
#include "vsfs/masterd/master_controller.h"

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
    for (auto& controller : cluster_) {
      controller->stop();
    }
    for (auto& thd : threads_) {
      thd.join();
    }
  }

  /**
   * \brief Starts a Master cluster with num_masters machines.
   * \note the first node is the primary one.
   */
  void start_cluster(int num_masters) {
    cluster_.emplace_back(unique_ptr<MasterController>(
            new MasterController(true, tmpdir_->path())));
    threads_.emplace_back(
        thread(&MasterController::start, cluster_.back().get()));
    for (int i = 0; i < num_masters - 1; ++i) {
      cluster_.emplace_back(unique_ptr<MasterController>(
            new MasterController(false, tmpdir_->path())));
      threads_.emplace_back(
        thread(&MasterController::start, cluster_.back().get()));
    }
  }

  unique_ptr<vobla::TemporaryDirectory> tmpdir_;
  vector<unique_ptr<MasterController>> cluster_;
  vector<thread> threads_;  // Each thread runs a master controller.
};

TEST_F(MasterServerIntegrationTest, TestStartMasterServerCluster) {
  start_cluster(4);
  LOG(INFO) << "Fully started.";
}

}  // namespace masterd
}  // namespace vsfs
