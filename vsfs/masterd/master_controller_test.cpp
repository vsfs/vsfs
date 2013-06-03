/**
 * \file master_controller_test.cpp
 *
 * \brief Unit tests for MasterController.
 *
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
 */

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/common/hash_util.h"
#include "vsfs/index/index_info.h"
#include "vsfs/masterd/master_controller.h"
#include "vsfs/rpc/vsfs_types.h"

using ::testing::Contains;
using ::testing::Pair;
using ::testing::Key;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::Status;
using vsfs::index::IndexInfo;

namespace vsfs {
namespace masterd {

static const uint64_t kNumPartitions = 4;
static const uint64_t kPartitonSize =
    std::numeric_limits<uint64_t>::max() / kNumPartitions;

/// Unit tests for Master Server.
class MasterControllerTest : public ::testing::Test {
 protected:
  void SetUp() {
    controller_.reset(new MasterController);
  }

  /// Evenly divide the C.H ring into 'num_index_servers' segements and let
  /// each index server manage one segment.
  void join_index_servers(int num_index_servers) {
    for (int i = 0; i < num_index_servers; ++i) {
      RpcNodeAddressList replicas;
      NodeInfo index_server;
      index_server.address.host = "192.168.1." + to_string(i);
      index_server.address.port = 5550 + i;
      ASSERT_TRUE(controller_->join_index_server(
          index_server, &replicas).ok());
    }
  }

  // Test Master Server instance.
  unique_ptr<MasterController> controller_;
};

}  // namespace masterd
}  // namespace vsfs
