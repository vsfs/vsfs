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
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vsfs/masterd/index_path_map.h"

using ::testing::ElementsAre;
using std::string;
using std::vector;

namespace vsfs {
namespace masterd {

TEST(IndexPathMap, TestCreates) {
  IndexPathMap test_map;

  Status status;
  EXPECT_TRUE(test_map.insert("/foo/bar", "test").ok());
  EXPECT_EQ(-EEXIST, test_map.insert("/foo/bar", "test").error());
  EXPECT_EQ(-EEXIST, test_map.insert("/foo/bar/", "test").error());
  EXPECT_EQ(-EEXIST, test_map.insert("/foo/bar///", "test").error());

  EXPECT_TRUE(test_map.insert("/", "test").ok());
  EXPECT_EQ(-EEXIST, test_map.insert("/", "test").error());
}

TEST(IndexPathMap, TestGet) {
  IndexPathMap test_map;

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

TEST(IndexPathMap, TestCollect) {
  IndexPathMap test_map;
  test_map.insert("/foo/bar", "test");
  test_map.insert("/foo/bar/data0", "data0");
  test_map.insert("/foo/bar/data1", "data1");
  test_map.insert("/foo/bar/data1", "test");

  vector<string> indices;
  // The end conditon is: iter == nodes_.end().
  EXPECT_TRUE(test_map.collect("/foo/bar", "test", &indices).ok());
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // The end conditon is: !starts_with(path, root).
  test_map.insert("/foo/sushi", "test");
  indices.clear();
  EXPECT_TRUE(test_map.collect("/foo/bar", "test", &indices).ok());
  EXPECT_THAT(indices, ElementsAre("/foo/bar", "/foo/bar/data1"));

  // Search from /foo
  indices.clear();
  test_map.insert("/foo/cdef", "cdef");
  EXPECT_TRUE(test_map.collect("/foo", "test", &indices).ok());
  EXPECT_THAT(indices,
              ElementsAre("/foo/bar", "/foo/bar/data1", "/foo/sushi"));
}

TEST(IndexPathMap, TestGetIndexNames) {
  IndexPathMap test_map;
  test_map.insert("/foo/bar", "abc");
  test_map.insert("/foo/bar", "def");
  test_map.insert("/foo/bar", "ghi");
  test_map.insert("/foo/bar", "jkl");

  vector<string> names;
  EXPECT_TRUE(test_map.get_index_names("/foo/bar", &names).ok());
  EXPECT_THAT(names, ElementsAre("abc", "def", "ghi", "jkl"));

  EXPECT_FALSE(test_map.get_index_names("/foo", &names).ok());
  EXPECT_FALSE(test_map.get_index_names("/", &names).ok());
  EXPECT_FALSE(test_map.get_index_names("/foo/bar/zoo", &names).ok());
}

}  // namespace masterd
}  // namespace vsfs
