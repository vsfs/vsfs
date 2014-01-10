/*
 * Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
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
#include "vsfs/ui/cli/commands.h"

namespace vsfs {
namespace ui {
namespace cli {

TEST(IndexInsertCommandTest, TestParseRecordFromStdin) {
  IndexInsertCommand cmd;
  string path, key;
  EXPECT_TRUE(cmd.parse_record("/foo/bar/zoo key", &path, &key));
  EXPECT_EQ("/foo/bar/zoo", path);
  EXPECT_EQ("key", key);

  path.clear();
  key.clear();
  EXPECT_TRUE(cmd.parse_record(" /foo/bar/zoo key ", &path, &key));
  EXPECT_EQ("/foo/bar/zoo", path);
  EXPECT_EQ("key", key);

  path.clear();
  key.clear();
  EXPECT_TRUE(cmd.parse_record("'/foo/bar/zoo' 'key'", &path, &key));
  EXPECT_EQ("/foo/bar/zoo", path);
  EXPECT_EQ("key", key);

  path.clear();
  key.clear();
  EXPECT_TRUE(cmd.parse_record("'/foo/bar zoo' 'key'", &path, &key));
  EXPECT_EQ("/foo/bar zoo", path);
  EXPECT_EQ("key", key);
}

}  // namespace cli
}  // namespace ui
}  // namespace vsfs
