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

#ifndef VSFS_MASTERD_TESTING_TEST_MASTERD_CLUSTER_H_
#define VSFS_MASTERD_TESTING_TEST_MASTERD_CLUSTER_H_

#include <boost/utility.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "vobla/status.h"
#include "vsfs/masterd/master_controller.h"

using std::string;
using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::Status;

namespace vsfs {
namespace masterd {

/**
 * \class TestMasterdCluster
 * \brief Constructs and manages a test masterd cluster locally for test
 * purpose.
 */
class TestMasterdCluster : boost::noncopyable {
 public:
  /**
   * \brief Constructs a Masterd cluster with 'num_masterd' nodes. The first
   * node will be the primary one.
   */
  TestMasterdCluster(const string& basedir, int cluster_size);

  /// Destructs the cluster and stop / reclaim resources.
  virtual ~TestMasterdCluster();

  /// Starts the cluster.
  void start();

  /// Stops the cluster.
  void stop();

  /// Returns the instance of the primary masterd.
  MasterController* primary();

  /// Returns the number of masterd node in this cluster.
  int cluster_size() const;

 private:
  string basedir_;

  int num_masterds_;

  vector<unique_ptr<MasterController>> cluster_;

  /// Threads to run MasterController.
  vector<thread> threads_;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_TESTING_TEST_MASTERD_CLUSTER_H_
