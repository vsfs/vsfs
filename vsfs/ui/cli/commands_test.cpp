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
#include <vector>
#include "vsfs/ui/cli/commands.h"

using std::string;
using std::vector;

namespace vsfs {
namespace ui {
namespace cli {

vector<char*> create_args(const vector<string>& args) {
  vector<char*> ret;
  for (const auto& arg : args) {
    ret.push_back(const_cast<char*>(arg.c_str()));
  }
  return ret;
}

TEST(IndexUpdateCommandTest, TestParseArgs) {
  IndexUpdateCommand cmd(IndexUpdateCommand::IndexOp::UPDATE);
  vector<string> strargs = {"insert", "--stdin", "energy"};
  auto args = create_args(strargs);
  EXPECT_EQ(0, cmd.parse_args(args.size(), &args[0]));
  EXPECT_TRUE(cmd.use_stdin_);
  EXPECT_EQ("energy", cmd.name_);
  EXPECT_EQ(IndexUpdateCommand::IndexOp::UPDATE, cmd.op_);

  IndexUpdateCommand cmd1(IndexUpdateCommand::IndexOp::UPDATE);
  strargs = {"insert", "energy", "file0", "key0", "file1", "key1"};
  args = create_args(strargs);
  EXPECT_EQ(0, cmd1.parse_args(args.size(), &args[0]));
  EXPECT_EQ("energy", cmd1.name_);
  EXPECT_EQ(2, cmd1.index_data_.size());

  // Missing file,key pair.
  IndexUpdateCommand cmd2(IndexUpdateCommand::IndexOp::UPDATE);
  strargs = {"insert", "energy", "file0", "key0", "file1", "key1", "file2"};
  args = create_args(strargs);
  EXPECT_EQ(-1, cmd2.parse_args(args.size(), &args[0]));
}

TEST(IndexUpdateCommandTest, TestParseRecordFromStdin) {
  IndexUpdateCommand cmd(IndexUpdateCommand::IndexOp::UPDATE);
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
