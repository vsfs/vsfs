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

#include <boost/locale.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vobla/status.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/common/mock_leveldb_store.h"
#include "vsfs/masterd/index_namespace.h"

using ::testing::ElementsAre;
using ::testing::Return;
using ::testing::_;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;

namespace vsfs {
namespace masterd {

class IndexNamespaceTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
    testdb_ = tmpdir_->path() + "/test.db";
    mock_db_ = new MockLevelDBStore;
    test_ns_.reset(new IndexNamespace(mock_db_));
  }

  void TearDown() {
    // Guarantees that the testing IndexNamespace object is destoryed before
    // the tmpdir_.
    test_ns_.reset();
  }

  string testdb_;
  unique_ptr<TemporaryDirectory> tmpdir_;
  unique_ptr<IndexNamespace> test_ns_;
  /// A pointe to the mock LevelDBStore. It does not own the MockLevelDBStore
  /// object.
  MockLevelDBStore* mock_db_;
};

TEST_F(IndexNamespaceTest, TestCreates) {
  // Only two successfully insertations count.
  EXPECT_CALL(*mock_db_, put(_, _))
      .Times(2)
      .WillRepeatedly(Return(Status::OK));
  EXPECT_TRUE(test_ns_->insert("/foo/bar", "test").ok());
  EXPECT_EQ(-EEXIST, test_ns_->insert("/foo/bar", "test").error());
  EXPECT_EQ(-EEXIST, test_ns_->insert("/foo/bar/", "test").error());
  EXPECT_EQ(-EEXIST, test_ns_->insert("/foo/bar///", "test").error());

  EXPECT_TRUE(test_ns_->insert("/", "test").ok());
  EXPECT_EQ(-EEXIST, test_ns_->insert("/", "test").error());
}

TEST_F(IndexNamespaceTest, TestCreateNameInUTF8) {
  const string path(u8"/这是一个目录");
  const string name(u8"索引一");
  {
    IndexNamespace test_ns(testdb_);
    test_ns.init();
        EXPECT_TRUE(test_ns.insert(path, name).ok());
    EXPECT_TRUE(test_ns.have(path, name));
  }

  {
    IndexNamespace test_ns(testdb_);
    test_ns.init();
    EXPECT_TRUE(test_ns.have(path, name));
  }
}

TEST_F(IndexNamespaceTest, TestInsertFailures) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillOnce(Return(Status(-1, "Mock failure")));
  auto status = test_ns_->insert("/foo", "bar");
  EXPECT_EQ(-1, status.error());
  EXPECT_EQ("Mock failure", status.message());
  // The inserted item should be rolled back.
  EXPECT_FALSE(test_ns_->have("/foo", "bar"));
}

TEST_F(IndexNamespaceTest, TestRemove) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  test_ns_->insert("/foo/bar", "test0");
  test_ns_->insert("/foo/bar", "test1");
  EXPECT_EQ(-ENOENT, test_ns_->remove("/foo", "test0").error());
  EXPECT_EQ(-ENOENT, test_ns_->remove("/foo/bar", "test2").error());

  EXPECT_TRUE(test_ns_->remove("/foo/bar", "test0").ok());
  EXPECT_FALSE(test_ns_->have("/foo/bar", "test0"));
  EXPECT_TRUE(test_ns_->have("/foo/bar", "test1"));
}

TEST_F(IndexNamespaceTest, TestRemoveADirectory) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  test_ns_->insert("/foo/bar", "test0");
  test_ns_->insert("/foo/bar", "test1");
  test_ns_->insert("/foo/bar", "test2");

  EXPECT_CALL(*mock_db_, remove("/foo/bar"))
      .WillOnce(Return(Status::OK));
  EXPECT_TRUE(test_ns_->remove("/foo/bar").ok());
  EXPECT_FALSE(test_ns_->have("/foo/bar", "test0"));
  EXPECT_FALSE(test_ns_->have("/foo/bar", "test1"));
  EXPECT_FALSE(test_ns_->have("/foo/bar", "test2"));
}

TEST_F(IndexNamespaceTest, TestGet) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));

  EXPECT_TRUE(test_ns_->insert("/foo/bar", "test").ok());
  test_ns_->insert("/foo/bar/zoo", "nontest");
  test_ns_->insert("/", "test");

  string test_path;
  EXPECT_TRUE(test_ns_->find("/foo/bar/zoo/test.txt", "test", &test_path).ok());
  EXPECT_EQ("/foo/bar", test_path);
  EXPECT_TRUE(test_ns_->find("/foo/bar", "test", &test_path).ok());
  EXPECT_EQ("/foo/bar", test_path);
  EXPECT_FALSE(test_ns_->find("/foo/bar/zoo/test.txt", "nowhere",
                             &test_path).ok());

  EXPECT_TRUE(test_ns_->find("/foo/fruit", "test", &test_path).ok());
  EXPECT_EQ("/", test_path);
}

TEST_F(IndexNamespaceTest, TestCollect) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  test_ns_->insert("/foo/bar", "test");
  test_ns_->insert("/foo/bar/data0", "data0");
  test_ns_->insert("/foo/bar/data1", "data1");
  test_ns_->insert("/foo/bar/data1", "test");

  auto indices = test_ns_->collect("/foo/bar", "test");
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // The end conditon is: !starts_with(path, root).
  test_ns_->insert("/foo/sushi", "test");
  indices = test_ns_->collect("/foo/bar", "test");
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // Search from /foo
  test_ns_->insert("/foo/cdef", "cdef");
  indices = test_ns_->collect("/foo", "test");
  EXPECT_THAT(indices,
              ElementsAre("/foo/bar", "/foo/bar/data1", "/foo/sushi"));
}

TEST_F(IndexNamespaceTest, TestGetIndicesRecursively) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  test_ns_->insert("/foo/bar", "test");
  test_ns_->insert("/foo/bar/data0", "data0");
  test_ns_->insert("/foo/bar/data1", "test");
  auto indices = test_ns_->get_indices("/foo", true);
  EXPECT_THAT(indices, ElementsAre("/foo/bar/.vsfs/test",
                                   "/foo/bar/data0/.vsfs/data0",
                                   "/foo/bar/data1/.vsfs/test"));
}

TEST_F(IndexNamespaceTest, TestGetIndexNames) {
  EXPECT_CALL(*mock_db_, put(_, _))
      .WillRepeatedly(Return(Status::OK));
  test_ns_->insert("/foo/bar", "abc");
  test_ns_->insert("/foo/bar", "def");
  test_ns_->insert("/foo/bar", "ghi");
  test_ns_->insert("/foo/bar", "jkl");

  auto names = test_ns_->get_index_names("/foo/bar");
  EXPECT_THAT(names, ElementsAre("abc", "def", "ghi", "jkl"));

  EXPECT_TRUE(test_ns_->get_index_names("/foo").empty());
  EXPECT_TRUE(test_ns_->get_index_names("/").empty());
  EXPECT_TRUE(test_ns_->get_index_names("/foo/bar/zoo").empty());
}

TEST_F(IndexNamespaceTest, TestReloadFromFileSystem) {
  {
    IndexNamespace test_ns(testdb_);
    EXPECT_TRUE(test_ns.init().ok());
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 10; j++) {
        test_ns.insert(string("/path") + to_string(i),
                       string("name") + to_string(j));
      }
    }
  }

  {
    IndexNamespace test_ns(testdb_);
    EXPECT_TRUE(test_ns.init().ok());
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 10; j++) {
        test_ns.have(string("/path") + to_string(i),
                     string("name") + to_string(j));
      }
    }
  }
}

}  // namespace masterd
}  // namespace vsfs
