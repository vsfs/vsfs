/*
 * Copyright 2013 (c) Ziling Huang <hzlgis@gmail.com>
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
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vobla/status.h"
#include "vsfs/index/index_info.h"
#include "vsfs/indexd/index_controller.h"
#include "vsfs/rpc/mock_rpc_clients.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::_;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::Status;
using vsfs::rpc::MockMasterServerClient;
using vsfs::rpc::TestRpcClientFactory;

namespace vsfs {
namespace indexd {

class IndexControllerTest : public ::testing::Test {
  typedef RpcClient<MasterServerClient, TFramedTransport> MasterClientType;
 protected:
  void SetUp() {
    shared_ptr<MockMasterServerClient> mock_master_(new MockMasterServerClient);
    unique_ptr<MasterClientType> master(new MasterClientType(mock_master_));

    tmpdir_.reset(new vobla::TemporaryDirectory);
    controller_.reset(new IndexController(tmpdir_->path(), "", 9876,
                                          master.release()));
  }

  void TearDown() {
  }

  unique_ptr<IndexController> controller_;
  MockMasterServerClient* mock_master_;
  unique_ptr<vobla::TemporaryDirectory> tmpdir_;
};

TEST_F(IndexControllerTest, TestCreateIndex) {
  EXPECT_TRUE(controller_->create_index("/foo/bar", "test", IndexInfo::BTREE,
                                        UINT64).ok());
}

}  // namespace indexd
}  // namespace vsfs
