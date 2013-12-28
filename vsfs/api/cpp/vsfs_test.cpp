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
#include <gmock/gmock.h>
#include <memory>
#include "vsfs/api/cpp/vsfs.h"
#include "vsfs/client/mock_vsfs_client.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/common/file.h"
#include "vsfs/common/mock_storage_manager.h"

using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;
using std::unique_ptr;

namespace vsfs {

using client::MockVSFSClient;

/**
 * \brief Unit tests for Vsfs C++ API.
 *
 * \note It is unit tests, but not integration tests.
 */
class VsfsAPITest : public ::testing::Test {
 protected:
  void SetUp() {
    mock_client_ = new MockVSFSClient;
    mock_sm_.reset(new MockStorageManager);
    vsfs_.reset(new Vsfs(mock_client_, mock_sm_.get()));
  }

  MockVSFSClient *mock_client_;
  unique_ptr<MockStorageManager> mock_sm_;
  unique_ptr<Vsfs> vsfs_;
};

TEST_F(VsfsAPITest, TestConnect) {
  EXPECT_CALL(*mock_client_, create("/abc", 10, 10, 10, NotNull()))
      .WillOnce(DoAll(SetArgPointee<4>(10),
                      Return(Status::OK)));
  EXPECT_CALL(*mock_sm_, open("/abc", _, _, _, NotNull()))
      .WillOnce(Return(Status::OK));
  File* file;
  EXPECT_TRUE(vsfs_->create("/abc", 10, 10, 10, &file).ok());
}

}  // namespace vsfs
