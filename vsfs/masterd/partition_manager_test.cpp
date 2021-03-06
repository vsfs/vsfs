/*
 * Copyright 2011-2013 (c) Lei Xu <eddyxu@gmail.com>
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

#include <boost/filesystem.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/masterd/partition_manager.h"

namespace fs = boost::filesystem;
using ::testing::ContainerEq;
using ::testing::ElementsAre;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

namespace vsfs {
namespace masterd {

class PartitionManagerTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    tmpdir_.reset(new vobla::TemporaryDirectory);
    testdir_ = tmpdir_->path();
  }

  virtual void TearDown() {
  }

  unique_ptr<vobla::TemporaryDirectory> tmpdir_;
  string testdir_;
};

TEST_F(PartitionManagerTest, TestInitialize) {
  string dbpath = testdir_ + "/test.db";
  PartitionManager manager(dbpath);
  EXPECT_FALSE(fs::exists(dbpath));
  EXPECT_TRUE(manager.init().ok());
  EXPECT_TRUE(fs::exists(dbpath));
}

TEST_F(PartitionManagerTest, TestAddIndex) {
  PartitionManager manager(testdir_ + "/test1.db");
  manager.init();
  string index_path = "/home/john/.vsfs/energy";
  EXPECT_TRUE(manager.add_index(index_path).ok());
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, 0));
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, -1000));

  EXPECT_FALSE(manager.add_index(index_path).ok());

  EXPECT_TRUE(manager.remove_index(index_path).ok());
  EXPECT_EQ("", manager.get_partition_path(index_path, 0));
}

TEST_F(PartitionManagerTest, TestRestoreFromDisk) {
  string path = testdir_ + "/db";
  {
    PartitionManager manager(path);
    manager.init();
    EXPECT_TRUE(manager.add_index("/test0").ok());
    EXPECT_TRUE(manager.add_partition("/test0", 1000).ok());
    EXPECT_TRUE(manager.add_index("/test1").ok());
    EXPECT_TRUE(manager.add_index("/test2").ok());
  }

  {
    PartitionManager manager(path);
    EXPECT_TRUE(manager.init().ok());
    EXPECT_EQ("/test0.0", manager.get_partition_path("/test0", 0));
    EXPECT_EQ("/test0.1000", manager.get_partition_path("/test0", -100));
    EXPECT_EQ("/test1.0", manager.get_partition_path("/test1", 65535));
    EXPECT_EQ("/test2.0", manager.get_partition_path("/test2", 10000));
    EXPECT_EQ("", manager.get_partition_path("/nonexist", 0));
  }
}

TEST_F(PartitionManagerTest, TestGetAllPartitions) {
  string path = testdir_ + "/test";
  PartitionManager manager(path);
  manager.init();
  manager.add_index("/test0");
  for (int64_t i = 1; i <= 10; ++i) {
    EXPECT_TRUE(manager.add_partition("/test0", i * 1000).ok());
  }

  vector<string> partitions;
  EXPECT_TRUE(manager.get_all_partitions("/test0", &partitions).ok());

  vector<string> expected_partitions;
  for (int64_t i = 0; i <= 10; ++i) {
    expected_partitions.emplace_back("/test0." + to_string(i * 1000));
  }
  EXPECT_THAT(partitions, ContainerEq(expected_partitions));
}

TEST_F(PartitionManagerTest, TestRemovePartition) {
  PartitionManager manager(testdir_ + "/test1.db");
  manager.init();

  manager.add_index("test");
  manager.add_partition("test", 100);
  manager.add_partition("test", 200);

  vector<string> partitions;
  manager.get_all_partitions("test", &partitions);
  EXPECT_THAT(partitions, ElementsAre("test.0", "test.100", "test.200"));

  EXPECT_FALSE(manager.remove_partition("test", 120).ok());
  EXPECT_TRUE(manager.remove_partition("test", 100).ok());
  partitions.clear();
  manager.get_all_partitions("test", &partitions);
  EXPECT_THAT(partitions, ElementsAre("test.0", "test.200"));
}

}  // namespace masterd
}  // namespace vsfs
