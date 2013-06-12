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
#include <set>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vobla/status.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/common/mock_leveldb_store.h"
#include "vsfs/metad/meta_manager.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::ContainerEq;
using ::testing::Return;
using ::testing::_;
using std::set;
using std::string;
using std::to_string;
using std::vector;
using vobla::TemporaryDirectory;

namespace vsfs {
namespace metad {

class MetaManagerTest : public ::testing::Test {
  protected:
    void SetUp() {
      tmpdir_.reset(new TemporaryDirectory);
      testdb_ = tmpdir_->path() + "/test.db";
      mock_db_ = new MockLevelDBStore;
      test_mm_.reset(new MetaManager(mock_db_));
    }

    void TearDown() {
      test_mm_.reset();
    }

    string testdb_;

    unique_ptr<TemporaryDirectory> tmpdir_;

    unique_ptr<MetaManager> test_mm_;

    MockLevelDBStore* mock_db_;
};

TEST_F(MetaManagerTest, TestInsertSingle) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .Times(2)
      .WillRepeatedly(Return(Status::OK));
  EXPECT_TRUE(test_mm_->insert(1, "/foo/bar/").ok());
  EXPECT_EQ(-EEXIST, test_mm_->insert(1, "/foo/bar/").error());
  EXPECT_TRUE(test_mm_->insert(2, "/foo/bar/").ok());
  EXPECT_EQ(-EEXIST, test_mm_->insert(2, "/foo/bar/").error());
}

TEST_F(MetaManagerTest, TestInsertVector) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .Times(2)
      .WillRepeatedly(Return(Status::OK));
  RpcMetaDataList file_vector;
  RpcMetaData file1;
  RpcMetaData file2;
  file1.file_id = 1;
  file1.file_path = "/foo/bar/";
  file2.file_id = 2;
  file2.file_path = "/foo/zoo/";
  file_vector.push_back(file1);
  file_vector.push_back(file2);
  EXPECT_TRUE(test_mm_->insert(file_vector).ok());
  EXPECT_EQ(-EEXIST, test_mm_->insert(1, "/foo/bar/").error());
  EXPECT_EQ(-EEXIST, test_mm_->insert(2, "/foo/zoo/").error());
}

TEST_F(MetaManagerTest, TestInsertFailure) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillOnce(Return(Status(-1, "Mock Failure")));
  auto status = test_mm_->insert(1, "/foo/bar/");
  EXPECT_EQ(-1, status.error());
  EXPECT_EQ("Mock Failure", status.message());
  // The inserted item should be rolled back.
  EXPECT_FALSE(test_mm_->have(1, "/foo/bar/"));
}

TEST_F(MetaManagerTest, TestRemove) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .Times(2)
      .WillRepeatedly(Return(Status::OK));
  EXPECT_CALL(*mock_db_, remove(_))
      .Times(2)
      .WillRepeatedly(Return(Status::OK));
  test_mm_->insert(1, "/foo/bar");
  test_mm_->insert(2, "/foo/zoo");

  EXPECT_EQ(-ENOENT, test_mm_->remove(3).error());
  EXPECT_TRUE(test_mm_->remove(1).ok());
  EXPECT_TRUE(test_mm_->remove(2).ok());
  EXPECT_EQ(-ENOENT, test_mm_->remove(1).error());
  EXPECT_EQ(-ENOENT, test_mm_->remove(2).error());
}

TEST_F(MetaManagerTest, TestFind) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  string test_path;
  EXPECT_EQ(-ENOENT, test_mm_->find(3, &test_path).error());
  test_mm_->insert(1, "/foo/bar");
  test_mm_->insert(2, "/foo/zoo");

  EXPECT_TRUE(test_mm_->find(1, &test_path).ok());
  EXPECT_EQ("/foo/bar", test_path);
  EXPECT_TRUE(test_mm_->find(2, &test_path).ok());
  EXPECT_EQ("/foo/zoo", test_path);
  EXPECT_EQ(-ENOENT, test_mm_->find(3, &test_path).error());
}

TEST_F(MetaManagerTest, TestFindVector) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  vector<int64_t> file_ids;
  set<string> expected_files;

  for (int i = 0; i < 100; i++) {
    string filepath = string("/energy/file-") +
                      to_string(i) + ".txt";
    expected_files.insert(filepath);
    int64_t hash = HashUtil::file_path_to_hash(filepath);
    test_mm_->insert(hash, filepath);
    file_ids.push_back(hash);
  }

  vector<string> file_names;
  EXPECT_TRUE(test_mm_->find(file_ids, &file_names).ok());
  set<string> actual_files(file_names.begin(), file_names.end());
  EXPECT_THAT(actual_files, ContainerEq(expected_files));
}

TEST_F(MetaManagerTest, TestHave) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  test_mm_->insert(1, "/foo/bar").ok();
  EXPECT_TRUE(test_mm_->have(1, "/foo/bar"));
  EXPECT_FALSE(test_mm_->have(2, "/foo/bar"));
  EXPECT_FALSE(test_mm_->have(1, "/foo/zoo"));
}

TEST_F(MetaManagerTest, TestSize) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  test_mm_->insert(1, "/foo/bar").ok();
  test_mm_->insert(2, "/foo/zoo").ok();
  EXPECT_EQ(static_cast<size_t>(2), test_mm_->size());
}

TEST_F(MetaManagerTest, TestInit) {
  {
    MetaManager test_mm(testdb_);
    EXPECT_TRUE(test_mm.init().ok());
    for (int i = 0; i < 5; i++) {
        test_mm.insert(i, string("/path") + to_string(i));
    }
  }

  {
    MetaManager test_mm(testdb_);
    EXPECT_TRUE(test_mm.init().ok());
    for (int i = 0; i < 5; i++) {
        test_mm.have(i, string("/path") + to_string(i));
    }
  }
}

}  // namespace metad
}  // namespace vsfs
