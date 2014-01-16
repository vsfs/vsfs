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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>
#include <string>
#include "vsfs/common/mock_storage_manager.h"
#include "vsfs/common/posix_file_handler.h"
#include "vsfs/common/posix_storage_manager.h"
#include "vobla/file.h"

using vobla::TemporaryDirectory;
using std::string;
using std::unique_ptr;

namespace vsfs {

class PosixFileHandlerTest : public ::testing::Test {
 public:
  typedef unique_ptr<PosixFileHandler> Handler;

  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
    sm_.reset(new MockStorageManager);
  }

  unique_ptr<TemporaryDirectory> tmpdir_;
  unique_ptr<MockStorageManager> sm_;
};

TEST_F(PosixFileHandlerTest, TestConstruct) {
  string path = tmpdir_->path() + "/test";
  int fd = open(path.c_str(), O_CREAT, 0666);
  Handler handler(new PosixFileHandler(sm_.get(), fd));
  EXPECT_EQ(handler->fd(), fd);
  EXPECT_TRUE(handler->close().ok());
  EXPECT_EQ(handler->fd(), -1);
}

}  // namespace vsfs
