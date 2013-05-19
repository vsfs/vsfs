/**
 * \file index_partition_manager_test.cpp
 *
 * \brief Unit tests for IndexPartitionManager.
 *
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
 */

#include <boost/filesystem.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "vsfs/masterd/partition_manager.h"

namespace fs = boost::filesystem;
using ::testing::ElementsAre;
using std::string;
using std::vector;

namespace vsfs {
namespace masterd {

class IndexPartitionManagerTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    testdir_ = SysUtil::make_temp_dir("index_partition_manager");
  }

  virtual void TearDown() {
    fs::remove_all(testdir_);
  }

  string testdir_;
};

TEST_F(IndexPartitionManagerTest, TestInsertIndex) {
  IndexPartitionManager manager("");
  string index_path = "/home/john/.vsfs/energy";
  manager.insert(index_path);
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, 0));
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, -1000));

  manager.remove(index_path);
  EXPECT_EQ("", manager.get_partition_path(index_path, 0));
}

TEST_F(IndexPartitionManagerTest, TestAddPartition) {
  IndexPartitionManager manager("");
  string john = "/home/john/.vsfs/energy";
  string bob = "/home/bob/.vsfs/energy";
  manager.insert(john);
  manager.insert(bob);

  manager.add_partition(bob, 10000);
  manager.add_partition(bob, 30000);
  vector<string> paths;
  manager.get_all_partition_paths(bob, &paths);
  EXPECT_EQ(static_cast<size_t>(3), paths.size());
  EXPECT_THAT(paths, ElementsAre(bob + ".0", bob + ".10000", bob + ".30000"));

  EXPECT_EQ(bob + ".0", manager.get_partition_path(bob, 5000));
}

TEST_F(IndexPartitionManagerTest, TestSaveAndLoad) {
  const string test_db_path = testdir_ + "/test.db";
  const string john = "/home/john/.vsfs/energy";
  const string bob = "/home/bob/.vsfs/energy";

  {
    IndexPartitionManager manager(test_db_path);
    manager.insert(john);
    manager.insert(bob);
    manager.add_partition(bob, 10000);
    manager.add_partition(bob, 30000);

    EXPECT_TRUE(manager.save().ok());
  }

  {
    IndexPartitionManager manager(test_db_path);
    EXPECT_TRUE(manager.load().ok());

    vector<string> paths;
    manager.get_all_partition_paths(bob, &paths);
    EXPECT_EQ(static_cast<size_t>(3), paths.size());
    EXPECT_THAT(paths, ElementsAre(bob + ".0", bob + ".10000",
                                   bob + ".30000"));
  }
}

}  // namespace masterd
}  // namespace vsfs
