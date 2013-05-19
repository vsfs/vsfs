/**
 * \file index_partition_manager_test.cpp
 *
 * \brief Unit tests for PartitionManager.
 *
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
 */

#include <boost/filesystem.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vsfs/common/key_value_store.h"
#include "vsfs/masterd/partition_manager.h"

namespace fs = boost::filesystem;
using ::testing::ElementsAre;
using std::string;
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

TEST_F(PartitionManagerTest, TestInsertIndex) {
  PartitionManager manager("");
  string index_path = "/home/john/.vsfs/energy";
  manager.insert(index_path);
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, 0));
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, -1000));

  manager.remove(index_path);
  EXPECT_EQ("", manager.get_partition_path(index_path, 0));
}

}  // namespace masterd
}  // namespace vsfs
