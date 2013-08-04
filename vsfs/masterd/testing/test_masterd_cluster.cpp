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
#include <unistd.h>
#include <string>
#include "vsfs/masterd/testing/test_masterd_cluster.h"

namespace vsfs {
namespace masterd {

TestMasterdCluster::TestMasterdCluster(const string& dirpath, int nmasterd)
    : basedir_(dirpath), num_masterds_(nmasterd) {
}

TestMasterdCluster::~TestMasterdCluster() {
  stop();
}

void TestMasterdCluster::start() {
  const int kPrimaryPort = 10100;
  cluster_.emplace_back(unique_ptr<MasterController>(
          new MasterController(basedir_, "", kPrimaryPort, true)));
  threads_.emplace_back(
      thread(&MasterController::start, cluster_.back().get()));
  // std::this_thread::sleep_for(std::chrono::seconds(2));
  sleep(2);

  // Starts the secondary masters.
  for (int i = 1; i < num_masterds_; ++i) {
    cluster_.emplace_back(unique_ptr<MasterController>(
            new MasterController(basedir_, "", kPrimaryPort + i,
                                 false, "", kPrimaryPort)));
    threads_.emplace_back(
        thread(&MasterController::start, cluster_.back().get()));
  }
}

void TestMasterdCluster::stop() {
  /// Stops the cluster and wait all threads exit.
  for (auto it = cluster_.rbegin(); it != cluster_.rend(); ++it) {
    (*it)->stop();
  }
  for (auto& thd : threads_) {
    if (thd.joinable()) {
      thd.join();
    }
  }
}

MasterController* TestMasterdCluster::primary() {
  CHECK_GE(cluster_.size(), 1);
  return cluster_[0].get();
}

int TestMasterdCluster::cluster_size() const {
  return cluster_.size();
}

}  // namespace masterd
}  // namespace vsfs
