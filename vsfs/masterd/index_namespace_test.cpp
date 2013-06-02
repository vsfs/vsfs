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
  }

  string testdb_;
  unique_ptr<TemporaryDirectory> tmpdir_;
};

// TODO(eddyxu): replace leveldb with MockLevelDBStore
TEST_F(IndexNamespaceTest, TestCreates) {
  IndexNamespace test_ns(testdb_);
  EXPECT_TRUE(test_ns.init().ok());

  EXPECT_TRUE(test_ns.insert("/foo/bar", "test").ok());
  EXPECT_EQ(-EEXIST, test_ns.insert("/foo/bar", "test").error());
  EXPECT_EQ(-EEXIST, test_ns.insert("/foo/bar/", "test").error());
  EXPECT_EQ(-EEXIST, test_ns.insert("/foo/bar///", "test").error());

  EXPECT_TRUE(test_ns.insert("/", "test").ok());
  EXPECT_EQ(-EEXIST, test_ns.insert("/", "test").error());
}

TEST_F(IndexNamespaceTest, TestInsertFailures) {
  MockLevelDBStore *mock_db = new MockLevelDBStore;
  IndexNamespace test_ns(mock_db);

  EXPECT_CALL(*mock_db, put(_, _))
      .WillOnce(Return(Status(-1, "Mock failure")));
  auto status = test_ns.insert("/foo", "bar");
  EXPECT_EQ(-1, status.error());
  EXPECT_EQ("Mock failure", status.message());
  // The inserted item should be rolled back.
  EXPECT_FALSE(test_ns.have("/foo", "bar"));
}

TEST_F(IndexNamespaceTest, TestRemove) {
  IndexNamespace test_ns(testdb_);
  EXPECT_TRUE(test_ns.init().ok());

  test_ns.insert("/foo/bar", "test0");
  test_ns.insert("/foo/bar", "test1");
  EXPECT_EQ(-ENOENT, test_ns.remove("/foo", "test0").error());
  EXPECT_EQ(-ENOENT, test_ns.remove("/foo/bar", "test2").error());

  EXPECT_TRUE(test_ns.remove("/foo/bar", "test0").ok());
  EXPECT_FALSE(test_ns.have("/foo/bar", "test0"));
  EXPECT_TRUE(test_ns.have("/foo/bar", "test1"));
}

TEST_F(IndexNamespaceTest, TestRemoveADirectory) {
  IndexNamespace test_ns(testdb_);
  EXPECT_TRUE(test_ns.init().ok());
  test_ns.insert("/foo/bar", "test0");
  test_ns.insert("/foo/bar", "test1");
  test_ns.insert("/foo/bar", "test2");

  EXPECT_TRUE(test_ns.remove("/foo/bar").ok());
  EXPECT_FALSE(test_ns.have("/foo/bar", "test0"));
  EXPECT_FALSE(test_ns.have("/foo/bar", "test1"));
  EXPECT_FALSE(test_ns.have("/foo/bar", "test2"));
}

TEST_F(IndexNamespaceTest, TestGet) {
  IndexNamespace test_ns(testdb_);
  test_ns.init();

  EXPECT_TRUE(test_ns.insert("/foo/bar", "test").ok());
  test_ns.insert("/foo/bar/zoo", "nontest");
  test_ns.insert("/", "test");

  string test_path;
  EXPECT_TRUE(test_ns.find("/foo/bar/zoo/test.txt", "test", &test_path).ok());
  EXPECT_EQ("/foo/bar", test_path);
  EXPECT_TRUE(test_ns.find("/foo/bar", "test", &test_path).ok());
  EXPECT_EQ("/foo/bar", test_path);
  EXPECT_FALSE(test_ns.find("/foo/bar/zoo/test.txt", "nowhere",
                             &test_path).ok());

  EXPECT_TRUE(test_ns.find("/foo/fruit", "test", &test_path).ok());
  EXPECT_EQ("/", test_path);
}

TEST_F(IndexNamespaceTest, TestCollect) {
  IndexNamespace test_ns(testdb_);
  test_ns.init();
  test_ns.insert("/foo/bar", "test");
  test_ns.insert("/foo/bar/data0", "data0");
  test_ns.insert("/foo/bar/data1", "data1");
  test_ns.insert("/foo/bar/data1", "test");

  auto indices = test_ns.collect("/foo/bar", "test");
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // The end conditon is: !starts_with(path, root).
  test_ns.insert("/foo/sushi", "test");
  indices = test_ns.collect("/foo/bar", "test");
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // Search from /foo
  test_ns.insert("/foo/cdef", "cdef");
  indices = test_ns.collect("/foo", "test");
  EXPECT_THAT(indices,
              ElementsAre("/foo/bar", "/foo/bar/data1", "/foo/sushi"));
}

TEST_F(IndexNamespaceTest, TestGetIndexNames) {
  IndexNamespace test_ns(testdb_);
  test_ns.init();
  test_ns.insert("/foo/bar", "abc");
  test_ns.insert("/foo/bar", "def");
  test_ns.insert("/foo/bar", "ghi");
  test_ns.insert("/foo/bar", "jkl");

  auto names = test_ns.get_index_names("/foo/bar");
  EXPECT_THAT(names, ElementsAre("abc", "def", "ghi", "jkl"));

  EXPECT_TRUE(test_ns.get_index_names("/foo").empty());
  EXPECT_TRUE(test_ns.get_index_names("/").empty());
  EXPECT_TRUE(test_ns.get_index_names("/foo/bar/zoo").empty());
}

}  // namespace masterd
}  // namespace vsfs