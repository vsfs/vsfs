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

#ifndef VSFS_TESTING_LOCAL_VSFS_CLUSTER_H_
#define VSFS_TESTING_LOCAL_VSFS_CLUSTER_H_

#include <boost/utility.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "vobla/status.h"

using std::string;
using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::Status;

namespace vsfs {

namespace indexd {
class IndexController;
}

namespace masterd {
class MasterController;
}

/**
 * \class LocalVSFSCluster
 * \brief Constructs and manages a test cluster locally for testing purpose.
 */
class LocalVsfsCluster : boost::noncopyable {
 public:
  /**
   * \brief Constructs a VSFS cluster locally and run each server in a separate
   * thread.
   * \param basedir the basedir to store all metadata/index/data.
   * \param num_masters the number of master servers to launch.
   * \param num_indices the number of index servers to launch.
   */
  LocalVsfsCluster(const string& basedir, int num_masters, int num_indices);

  /// Destructs the cluster and stop / reclaim resources.
  virtual ~LocalVsfsCluster();

  /// Starts the VSFS cluster.
  void start();

  /// Stops the VSFS cluster.
  void stop();

  /// Returns the instance of the primary masterd.
  masterd::MasterController* primary();

  /// Returns the host address of the `idx`-th masterd node.
  string host(int idx) const;

  /// Returns the listening port of the `idx`-th master node.
  int port(int idx) const;

  /// Returns the number of masterd node in this cluster.
  size_t num_master_servers() const;

 private:
  string basedir_;

  /// Number of master servers.
  int num_masterds_;

  /// Number of index servers.
  int num_index_servers_;

  vector<unique_ptr<masterd::MasterController>> master_servers_;

  vector<unique_ptr<indexd::IndexController>> index_servers_;

  /// Threads to run MasterController.
  vector<thread> master_threads_;

  /// Threads to run IndexController.
  vector<thread> index_threads_;
};

}  // namespace vsfs

#endif  // VSFS_TESTING_LOCAL_VSFS_CLUSTER_H_
