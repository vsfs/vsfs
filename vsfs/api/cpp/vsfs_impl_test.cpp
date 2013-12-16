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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include "vsfs/api/cpp/vsfs_impl.h"
#include "vsfs/client/mock_vsfs_client.h"
#include "vsfs/common/complex_query.h"

using std::unique_ptr;
using vsfs::client::MockVSFSClient;
using ::testing::Return;

namespace vsfs {
namespace api {

class VsfsImplTest : public ::testing::Test {
 protected:
  void SetUp() {
    mock_client_ = new MockVSFSClient;
    vsfs_.reset(new VsfsImpl(mock_client_));
  }

  void TearDown() {
    mock_client_ = nullptr;
  }

  MockVSFSClient* mock_client_;
  unique_ptr<VsfsImpl> vsfs_;
};

TEST_F(VsfsImplTest, TestConnect) {
  EXPECT_CALL(*mock_client_, init()).WillOnce(Return(Status::OK));
  EXPECT_TRUE(vsfs_->connect().ok());
}

}  // namespace api
}  // namespace vsfs
