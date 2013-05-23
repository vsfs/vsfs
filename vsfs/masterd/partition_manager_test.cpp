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
#include "vsfs/common/mock_key_value_store.h"
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
    mock_store_.reset(new MockKeyValueStore);
    testdir_ = tmpdir_->path();
  }

  virtual void TearDown() {
  }

  unique_ptr<vobla::TemporaryDirectory> tmpdir_;
  unique_ptr<MockKeyValueStore> mock_store_;
  string testdir_;
};

TEST_F(PartitionManagerTest, TestInsertIndex) {
  PartitionManager manager(testdir_ + "/test1.db");
  string index_path = "/home/john/.vsfs/energy";
  EXPECT_TRUE(manager.add_index(index_path).ok());
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, 0));
  EXPECT_EQ(index_path + ".0", manager.get_partition_path(index_path, -1000));

  manager.remove(index_path);
  EXPECT_EQ("", manager.get_partition_path(index_path, 0));
}

}  // namespace masterd
}  // namespace vsfs
