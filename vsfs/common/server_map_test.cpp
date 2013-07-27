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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/common/server_map.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::ContainerEq;
using std::map;
using std::string;
using std::to_string;
using std::vector;

namespace vsfs {
namespace masterd {

TEST(ServerMapTest, TestAddServer) {
  ServerMap test_map;
  vector<HashValueType> partitions;
  NodeInfo node1;
  NodeInfo node2;
  NodeInfo node3;
  NodeInfo node4;
  EXPECT_TRUE(test_map.add(node1).ok());
  EXPECT_EQ(1u, test_map.num_nodes());

  EXPECT_TRUE(test_map.add(node2).ok());
  EXPECT_EQ(2u, test_map.num_nodes());
  EXPECT_TRUE(test_map.add(node3).ok());
  EXPECT_EQ(3u, test_map.num_nodes());
  EXPECT_TRUE(test_map.add(node4).ok());
  EXPECT_EQ(4u, test_map.num_nodes());

  partitions = test_map.get_partitions();
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

TEST(ServerMapTest, TestRemoveIndexServer) {
  ServerMap test_map;
  NodeInfo node;
  node.address.host = "192.168.1.1";
  node.server_id = "node1";
  EXPECT_TRUE(test_map.add(0, node).ok());
  EXPECT_EQ(static_cast<size_t>(1), test_map.num_nodes());
  EXPECT_TRUE(test_map.remove(0).ok());
  EXPECT_EQ(static_cast<size_t>(0), test_map.num_nodes());
}

TEST(ServerMapTest, TestGetIndexServer) {
  ServerMap test_map;

  NodeInfo node1;
  NodeInfo node2;
  NodeInfo node3;
  NodeInfo node4;
  node1.server_id = "node1";
  node2.server_id = "node2";
  node3.server_id = "node3";
  node4.server_id = "node4";
  EXPECT_TRUE(test_map.add(node1).ok());
  EXPECT_TRUE(test_map.add(node2).ok());
  EXPECT_TRUE(test_map.add(node3).ok());
  EXPECT_TRUE(test_map.add(node4).ok());

  int node1_count = 0;
  int node2_count = 0;
  int node3_count = 0;
  int node4_count = 0;
  NodeInfo tmp;
  for (int i = 0; i < 2000; i++) {
    string path = "/tmp_data/test_data/" + std::to_string(i);
    int64_t file_id = HashUtil::file_path_to_hash(path);
    EXPECT_TRUE(test_map.get(file_id, &tmp).ok());
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

TEST(ServerMapTest, TestGetReplicaServers) {
  ServerMap test_map;
  for (int i = 0; i < 10; i++) {
    NodeInfo node;
    node.server_id = string("node") + to_string(i);
    test_map.add(i * 1000, node);
  }

  NodeInfo node;
  node.server_id = string("node2");
  auto replicas = test_map.get_replica_servers(node, 2);
  EXPECT_EQ(2u, replicas.size());
  EXPECT_EQ("node3", replicas[0].server_id);
  EXPECT_EQ("node4", replicas[1].server_id);
}

TEST(ServerMapTest, TestGetChRingAsMap) {
  ServerMap test_sm;
  map<HashValueType, NodeInfo> expected_map;
  for (int i = 0; i < 10; i++) {
    NodeInfo node;
    node.server_id = string("node") + to_string(i);
    test_sm.add(i * 1000, node);
    expected_map[i * 1000] = node;
  }

  auto actual_map = test_sm.get_ch_ring_as_map();
  EXPECT_THAT(actual_map, ContainerEq(expected_map));
}

}  // namespace masterd
}  // namespace vsfs
