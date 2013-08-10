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
#include <thread>  // NOLINT
#include "vsfs/indexd/index_controller.h"
#include "vsfs/masterd/master_controller.h"
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/testing/local_vsfs_cluster.h"

namespace fs = boost::filesystem;
using apache::thrift::transport::TTransportException;
using std::to_string;
using vsfs::indexd::IndexController;
using vsfs::masterd::MasterController;

namespace vsfs {

LocalVsfsCluster::LocalVsfsCluster(const string& dirpath, int num_masters,
                                   int num_indices)
    : basedir_(dirpath), num_masterds_(num_masters),
      num_index_servers_(num_indices) {
}

LocalVsfsCluster::~LocalVsfsCluster() {
  stop();
}

void LocalVsfsCluster::start() {
  typedef rpc::RpcClient<MasterServerClient> MasterClientType;
  const int kPrimaryPort = 10100;
  const int kIndexPort = 12000;
  fs::create_directories(basedir_ + "/m/0");
  master_servers_.emplace_back(unique_ptr<MasterController>(
          new MasterController(basedir_ + "/m/0", "", kPrimaryPort, true)));
  CHECK(master_servers_.back()->init().ok());
  master_threads_.emplace_back(
      thread(&MasterController::start, master_servers_.back().get()));
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
  VLOG(0) << "Primary masterd starts successfully.";

  // Starts the secondary masters.
  for (int i = 1; i < num_masterds_; ++i) {
    string basedir = basedir_ + "/m/" + std::to_string(i);
    fs::create_directories(basedir);
    master_servers_.emplace_back(unique_ptr<MasterController>(
            new MasterController(basedir, "", kPrimaryPort + i,
                                 false, "", kPrimaryPort)));

    CHECK(master_servers_.back()->init().ok());
    master_threads_.emplace_back(
        thread(&MasterController::start, master_servers_.back().get()));
  }
  while (true) {
    RpcConsistentHashRing all_masters;
    master_servers_[0]->get_all_masters(&all_masters);
    if (all_masters.size() == static_cast<size_t>(num_masterds_)) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }
  VLOG(0) << "Masterd(s) have fully started.";
  for (int i = 0; i < num_index_servers_; ++i) {
    string basedir = basedir_ + "/i/" + std::to_string(i);
    fs::create_directories(basedir);
    index_servers_.emplace_back(unique_ptr<IndexController>(
            new IndexController(basedir, "localhost", kIndexPort + i,
                                host(0), port(0))));
    // CHECK(index_servers_.back()->init().ok());
    index_threads_.emplace_back(
        thread(&IndexController::start, index_servers_.back().get()));
  }
  while (true) {
    // VLOG(0) << "Check all index servers are online?...";
    RpcConsistentHashRing all_index_servers;
    master_servers_[0]->get_all_index_servers(&all_index_servers);
    // VLOG(0) << "Online index servers: " << all_index_servers.size();
    if (all_index_servers.size() == static_cast<size_t>(num_index_servers_)) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }
}

void LocalVsfsCluster::stop() {
  /// Stops the cluster and wait all threads exit.
  for (auto& index_server : index_servers_) {
    index_server->stop();
  }
  for (auto& thd : index_threads_) {
    if (thd.joinable()) {
      thd.join();
    }
  }
  for (auto it = master_servers_.rbegin(); it != master_servers_.rend(); ++it) {
    (*it)->stop();
  }
  for (auto& thd : master_threads_) {
    if (thd.joinable()) {
      thd.join();
    }
  }
}

MasterController* LocalVsfsCluster::primary() {
  CHECK_GE(master_servers_.size(), 1);
  return master_servers_[0].get();
}

string LocalVsfsCluster::host(int idx) const {
  CHECK_LT(idx, master_servers_.size());
  return master_servers_[idx]->host();
}

int LocalVsfsCluster::port(int idx) const {
  CHECK_LT(idx, master_servers_.size());
  return master_servers_[idx]->port();
}

size_t LocalVsfsCluster::num_master_servers() const {
  return master_servers_.size();
}

}  // namespace vsfs
