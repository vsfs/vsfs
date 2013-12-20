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
#include "vsfs/api/cpp/vsfs_impl.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/client/mock_vsfs_client.h"

using std::unique_ptr;
using ::testing::_;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgPointee;

namespace vsfs {

using api::VsfsImpl;
using client::MockVSFSClient;

class VsfsAPITest : public ::testing::Test {
 protected:
  void SetUp() {
    mock_client_ = new MockVSFSClient;
    vsfs_.reset(new Vsfs(new VsfsImpl(mock_client_)));
  }

  MockVSFSClient *mock_client_;
  unique_ptr<Vsfs> vsfs_;
};

TEST_F(VsfsAPITest, TestConnect) {
  EXPECT_CALL(*mock_client_, create("/abc", 10, 10, 10, NotNull()))
      .WillOnce(DoAll(SetArgPointee<4>(10),
                      Return(Status::OK)));
  Vsfs::ObjectId id;
  EXPECT_TRUE(vsfs_->create("/abc", 10, 10, 10, &id).ok());
}

}  // namespace vsfs
