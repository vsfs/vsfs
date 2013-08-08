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
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <thrift/transport/TTransportException.h>
#include <unistd.h>
#include <chrono>
#include <string>
#include <thread>
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/masterd/testing/local_masterd_cluster.h"

namespace fs = boost::filesystem;
using apache::thrift::transport::TTransportException;

namespace vsfs {
namespace masterd {

LocalMasterdCluster::LocalMasterdCluster(const string& dirpath, int nmasterd)
    : basedir_(dirpath), num_masterds_(nmasterd) {
}

LocalMasterdCluster::~LocalMasterdCluster() {
  stop();
}

void LocalMasterdCluster::start() {
  typedef rpc::RpcClient<MasterServerClient> MasterClientType;
  const int kPrimaryPort = 10100;
  fs::create_directories(basedir_ + "/0");
  cluster_.emplace_back(unique_ptr<MasterController>(
          new MasterController(basedir_ + "/0", "", kPrimaryPort, true)));
  CHECK(cluster_.back()->init().ok());
  threads_.emplace_back(
      thread(&MasterController::start, cluster_.back().get()));
  while (true) {
    try {
      MasterClientType client("localhost", kPrimaryPort);
      client.open();
      client.close();
    } catch (TTransportException e) {  // NOLINT
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      continue;
    }
    break;
  }

  // Starts the secondary masters.
  for (int i = 1; i < num_masterds_; ++i) {
    string basedir = basedir_ + "/" + std::to_string(i);
    fs::create_directories(basedir);
    cluster_.emplace_back(unique_ptr<MasterController>(
            new MasterController(basedir, "", kPrimaryPort + i,
                                 false, "", kPrimaryPort)));

    CHECK(cluster_.back()->init().ok());
    threads_.emplace_back(
        thread(&MasterController::start, cluster_.back().get()));
  }
  while (true) {
    RpcConsistentHashRing all_masters;
    cluster_[0]->get_all_masters(&all_masters);
    if (all_masters.size() == static_cast<size_t>(num_masterds_)) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void LocalMasterdCluster::stop() {
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

MasterController* LocalMasterdCluster::primary() {
  CHECK_GE(cluster_.size(), 1);
  return cluster_[0].get();
}

string LocalMasterdCluster::host(int idx) const {
  CHECK_LT(idx, cluster_.size());
  return cluster_[idx]->host();
}

int LocalMasterdCluster::port(int idx) const {
  CHECK_LT(idx, cluster_.size());
  return cluster_[idx]->port();
}

int LocalMasterdCluster::cluster_size() const {
  return cluster_.size();
}

}  // namespace masterd
}  // namespace vsfs
