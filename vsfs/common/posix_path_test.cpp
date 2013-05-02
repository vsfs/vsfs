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

#include <gtest/gtest.h>
#include <string>
#include "vsfs/common/posix_path.h"

using std::string;

namespace vsfs {

TEST(PosixPathTest, TestRootDir) {
  PosixPath root("/");
  EXPECT_FALSE(root.is_query());
  EXPECT_EQ("/", root.path());
  EXPECT_EQ("/", root.root());
}

TEST(PosixPathTest, TestQuery) {
  PosixPath q("/home/eddyxu/?size>10");
  EXPECT_TRUE(q.is_query());
  EXPECT_EQ("/home/eddyxu", q.root());
  EXPECT_EQ("size>10", q.query());

  PosixPath q2("/home/bar/?size>10/");
  EXPECT_TRUE(q.is_query());
  EXPECT_EQ("/home/eddyxu", q.root());
  EXPECT_EQ("size>10", q.query());
}

TEST(PosixPathTest, TestIsResult) {
  PosixPath q("/home/foo/?size>100/abc.txt");
  EXPECT_FALSE(q.is_query());
  EXPECT_TRUE(q.is_result());
  EXPECT_EQ("size>100", q.query());
  EXPECT_EQ("abc.txt", q.result());
}

}  // namespace vsfs
