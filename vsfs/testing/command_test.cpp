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

/**
 * \file command_test.cpp
 * \brief Integration tests for vsfs utility command line.
 */

#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>
#include <stack>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/testing/local_vsfs_cluster.h"
#include "vsfs/ui/cli/commands.h"

using std::stack;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;
using vsfs::LocalVsfsCluster;
using vsfs::client::VSFSRpcClient;
namespace fs = boost::filesystem;

namespace vsfs {
namespace ui {
namespace cli {

class CommandTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
  }

  void TearDown() {
    cluster_.reset();
    tmpdir_.reset();
  }

  void start(int num_masters, int num_indices) {
    cluster_.reset(new LocalVsfsCluster(
        tmpdir_->path(), num_masters, num_indices));
    cluster_->start();

    client_.reset(new VSFSRpcClient(cluster_->host(0), cluster_->port(0)));
    EXPECT_TRUE(client_->init().ok());
  }

  /// Recursively creates all absent directories in 'path'.
  void create_directories(const string& path) {
    stack<string> parent_dirs;
    auto tmp = path;
    parent_dirs.push(tmp);
    while (tmp != "/") {
      tmp = fs::path(tmp).parent_path().string();
      parent_dirs.push(tmp);
    }
    while (!parent_dirs.empty()) {
      auto dir = parent_dirs.top();
      parent_dirs.pop();
      client_->mkdir(dir, 0755, 100, 100).ok();
    }
  }

  vector<char*> conv_to_argv(const vector<string>& params) {
    vector<char*> argv;
    argv.reserve(params.size());
    for (auto& param : params) {
      argv.push_back(const_cast<char*>(param.c_str()));
    }
    return argv;
  }

  unique_ptr<TemporaryDirectory> tmpdir_;
  unique_ptr<LocalVsfsCluster> cluster_;
  unique_ptr<VSFSRpcClient> client_;
};

TEST_F(CommandTest, TestCreateIndex) {
  start(2, 2);
  create_directories("/foo/bar");

  unique_ptr<Command> cmd(Command::create_command("index"));
  EXPECT_TRUE(cmd != nullptr);

  string port = to_string(cluster_->port(0));
  vector<string> params = { "index", "create", "-p", port.c_str(),
    "-n", "test", "-t", "btree", "-k", "uint32", "/foo/bar" };
  auto argv = conv_to_argv(params);
  EXPECT_EQ(0, cmd->parse_args(argv.size(), &argv[0]));
  EXPECT_TRUE(cmd->run().ok());
  struct stat stbuf;
  EXPECT_TRUE(client_->getattr("/foo/bar/.vsfs/test", &stbuf).ok());
}

}  // namespace cli
}  // namespace ui
}  // namespace vsfs
