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

#include <boost/filesystem.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include "vsfs/fuse/vsfs_ops.h"

using std::string;
using ::testing::Return;
using boost::filesystem::remove_all;

class VSFSOpsTest : public ::testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }

  string testdir_;
};

TEST_F(VSFSOpsTest, testCreate) {
}
