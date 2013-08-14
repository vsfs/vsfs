/*
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
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
#include "vsfs/common/path_util.h"

using std::string;

namespace vsfs {

TEST(PathUtilTest, TestSplitIndexPath) {
  string root, name;
  EXPECT_TRUE(PathUtil::split_index_path("/foo/bar/.vsfs/name", &root, &name));
  EXPECT_EQ("/foo/bar", root);
  EXPECT_EQ("name", name);

  EXPECT_FALSE(PathUtil::split_index_path("/foo/bar/.vsfs", &root, &name));
  EXPECT_FALSE(PathUtil::split_index_path("/foo/bar/.vsfs/", &root, &name));
  EXPECT_TRUE(PathUtil::split_index_path("/foo/bar/.vsfs/a", &root, &name));
  EXPECT_EQ("a", name);

  EXPECT_TRUE(PathUtil::split_index_path("/.vsfs/abc", &root, &name));
  EXPECT_EQ("/", root);
  EXPECT_EQ("abc", name);
}

}  // namespace vsfs
