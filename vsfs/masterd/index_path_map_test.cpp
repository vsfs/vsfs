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
#include "vsfs/masterd/index_path_map.h"

using ::testing::ElementsAre;
using std::string;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;

namespace vsfs {
namespace masterd {

class IndexPathMapTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
    testdb_ = tmpdir_->path() + "/test.db";
  }

  string testdb_;
  unique_ptr<TemporaryDirectory> tmpdir_;
};

// TODO(eddyxu): replace leveldb with MockLevelDBStore
TEST_F(IndexPathMapTest, TestCreates) {
  IndexPathMap test_map(testdb_);
  EXPECT_TRUE(test_map.init().ok());

  Status status;
  EXPECT_TRUE(test_map.insert("/foo/bar", "test").ok());
  EXPECT_EQ(-EEXIST, test_map.insert("/foo/bar", "test").error());
  EXPECT_EQ(-EEXIST, test_map.insert("/foo/bar/", "test").error());
  EXPECT_EQ(-EEXIST, test_map.insert("/foo/bar///", "test").error());

  EXPECT_TRUE(test_map.insert("/", "test").ok());
  EXPECT_EQ(-EEXIST, test_map.insert("/", "test").error());
}

TEST_F(IndexPathMapTest, TestGet) {
  IndexPathMap test_map(testdb_);
  test_map.init();

  Status status;
  EXPECT_TRUE(test_map.insert("/foo/bar", "test").ok());
  test_map.insert("/foo/bar/zoo", "nontest");
  test_map.insert("/", "test");

  string test_path;
  EXPECT_TRUE(test_map.find("/foo/bar/zoo/test.txt", "test", &test_path).ok());
  EXPECT_EQ("/foo/bar", test_path);
  EXPECT_TRUE(test_map.find("/foo/bar", "test", &test_path).ok());
  EXPECT_EQ("/foo/bar", test_path);
  EXPECT_FALSE(test_map.find("/foo/bar/zoo/test.txt", "nowhere",
                             &test_path).ok());

  EXPECT_TRUE(test_map.find("/foo/fruit", "test", &test_path).ok());
  EXPECT_EQ("/", test_path);
}

TEST_F(IndexPathMapTest, TestCollect) {
  IndexPathMap test_map(testdb_);
  test_map.init();
  test_map.insert("/foo/bar", "test");
  test_map.insert("/foo/bar/data0", "data0");
  test_map.insert("/foo/bar/data1", "data1");
  test_map.insert("/foo/bar/data1", "test");

  auto indices = test_map.collect("/foo/bar", "test");
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // The end conditon is: !starts_with(path, root).
  test_map.insert("/foo/sushi", "test");
  indices = test_map.collect("/foo/bar", "test");
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // Search from /foo
  test_map.insert("/foo/cdef", "cdef");
  indices = test_map.collect("/foo", "test");
  EXPECT_THAT(indices,
              ElementsAre("/foo/bar", "/foo/bar/data1", "/foo/sushi"));
}

TEST_F(IndexPathMapTest, TestGetIndexNames) {
  IndexPathMap test_map(testdb_);
  test_map.init();
  test_map.insert("/foo/bar", "abc");
  test_map.insert("/foo/bar", "def");
  test_map.insert("/foo/bar", "ghi");
  test_map.insert("/foo/bar", "jkl");

  auto names = test_map.get_index_names("/foo/bar");
  EXPECT_THAT(names, ElementsAre("abc", "def", "ghi", "jkl"));

  EXPECT_TRUE(test_map.get_index_names("/foo").empty());
  EXPECT_TRUE(test_map.get_index_names("/").empty());
  EXPECT_TRUE(test_map.get_index_names("/foo/bar/zoo").empty());
}

}  // namespace masterd
}  // namespace vsfs
