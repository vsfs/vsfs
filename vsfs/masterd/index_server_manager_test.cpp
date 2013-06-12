/*
 * Copyright 2013 (c) Ziling Huang <hzlgis@gmail.com>
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

#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/masterd/index_server_manager.h"
#include "vsfs/rpc/vsfs_types.h"

using std::string;
using std::to_string;
using std::vector;

namespace vsfs {
namespace masterd {

TEST(IndexServerManagerTest, TestAddIndexServer) {
  IndexServerManager test_idx;
  vector<size_t> partitions;
  NodeInfo node1;
  NodeInfo node2;
  NodeInfo node3;
  NodeInfo node4;
  EXPECT_TRUE(test_idx.add(node1).ok());
  EXPECT_EQ(static_cast<size_t>(1), test_idx.num_nodes());

  EXPECT_TRUE(test_idx.add(node2).ok());
  EXPECT_EQ(static_cast<size_t>(2), test_idx.num_nodes());
  EXPECT_TRUE(test_idx.add(node3).ok());
  EXPECT_EQ(static_cast<size_t>(3), test_idx.num_nodes());
  EXPECT_TRUE(test_idx.add(node4).ok());
  EXPECT_EQ(static_cast<size_t>(4), test_idx.num_nodes());

  partitions = test_idx.get_partitions();
  EXPECT_EQ(4u, partitions.size());
  size_t range;
  for (size_t i = 0; i < partitions.size(); i++) {
    if (i + 1 == partitions.size()) {
      range = partitions[0] + numeric_limits<size_t>::max() - partitions[i];
    } else {
      range = partitions[i+1] - partitions[i];
    }
    EXPECT_NEAR(1/16, range/numeric_limits<size_t>::max(), 0.01);
  }
}

TEST(IndexServerManagerTest, TestRemoveIndexServer) {
  IndexServerManager test_idx;
  NodeInfo node;
  node.address.host = "192.168.1.1";
  node.server_id = "node1";
  EXPECT_TRUE(test_idx.add(0, node).ok());
  EXPECT_EQ(static_cast<size_t>(1), test_idx.num_nodes());
  EXPECT_TRUE(test_idx.remove(0).ok());
  EXPECT_EQ(static_cast<size_t>(0), test_idx.num_nodes());
}

TEST(IndexServerManagerTest, TestGetIndexServer) {
  IndexServerManager test_idx;

  NodeInfo node1;
  NodeInfo node2;
  NodeInfo node3;
  NodeInfo node4;
  node1.server_id = "node1";
  node2.server_id = "node2";
  node3.server_id = "node3";
  node4.server_id = "node4";
  EXPECT_TRUE(test_idx.add(node1).ok());
  EXPECT_TRUE(test_idx.add(node2).ok());
  EXPECT_TRUE(test_idx.add(node3).ok());
  EXPECT_TRUE(test_idx.add(node4).ok());

  int node1_count = 0;
  int node2_count = 0;
  int node3_count = 0;
  int node4_count = 0;
  NodeInfo tmp;
  for (int i = 0; i < 2000; i++) {
    string path = "/tmp_data/test_data/" + std::to_string(i);
    uint64_t file_id = HashUtil::file_path_to_hash(path);
    EXPECT_TRUE(test_idx.get(file_id, &tmp).ok());
    if (tmp.server_id == "node1") {
      node1_count++;
    } else if (tmp.server_id == "node2") {
      node2_count++;
    } else if (tmp.server_id == "node3") {
      node3_count++;
    } else {
      node4_count++;
    }
  }
  /*EXPECT_NEAR(node1_count, 500, 50);
  EXPECT_NEAR(node2_count, 500, 50);
  EXPECT_NEAR(node3_count, 500, 50);
  EXPECT_NEAR(node4_count, 500, 50);*/
}

TEST(IndexServerManagerTest, TestGetReplicaServers) {
  IndexServerManager test_ism;
  for (int i = 0; i < 10; i++) {
    NodeInfo node;
    node.server_id = string("node") + to_string(i);
    test_ism.add(i * 1000, node);
  }

  NodeInfo node;
  node.server_id = string("node2");
  auto replicas = test_ism.get_replica_servers(node, 2);
  EXPECT_EQ(2u, replicas.size());
  EXPECT_EQ("node3", replicas[0].server_id);
  EXPECT_EQ("node4", replicas[1].server_id);
}

}  // namespace masterd
}  // namespace vsfs
