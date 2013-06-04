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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/file.h"

using vobla::Status;
using std::string;

namespace vsfs {

TEST(FileTest, TestConstructor) {
  File file;
  EXPECT_EQ(file.fd(), 0);
}

TEST(FileTest, TestOpen) {
  string f("/etc/hosts");
  File file;
  EXPECT_TRUE(file.open(f.c_str(), O_RDONLY).ok());
}

TEST(FileTest, TestClose) {
  string f("/etc/hosts");
  File file;
  EXPECT_TRUE(file.open(f.c_str(), O_RDONLY).ok());
  EXPECT_TRUE(file.close().ok());
}

TEST(FileTest, TestAssignmentOperator) {
  string f("/etc/hosts");
  File file;
  file.open(f.c_str(), O_RDONLY);
  int fd_old = file.fd();
  File other;
  other = file;
  EXPECT_EQ(fd_old, other.fd());
}

TEST(FileTest, TestMoveConstructor) {
  string f("/etc/hosts");
  File file;
  file.open(f.c_str(), O_RDONLY);
  int fd_old = file.fd();
  File other(file);
  EXPECT_EQ(fd_old, other.fd());
}

TEST(FileTest, TestCopyConstructor) {
  string f("/etc/hosts");
  File file;
  file.open(f.c_str(), O_RDONLY);
  int fd_old = file.fd();
  File other = file;
  EXPECT_EQ(fd_old, other.fd());
}

TEST(FileTest, TestRelease) {
  string f("/etc/hosts");
  File file;
  file.open(f.c_str(), O_RDONLY);
  int fd_old = file.fd();
  EXPECT_EQ(fd_old, file.release());
  EXPECT_EQ(file.fd(), 0);
}

}  // namespace vsfs
