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

/**
 * \file client_metadata_test.cpp
 * \brief Integration tests of the client to issues metadata ops to the master
 * cluster.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include "vobla/file.h"

using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;

namespace vsfs {

class ClientMetadataTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
  }

  void TearDown() {
  }

  unique_ptr<TemporaryDirectory> tmpdir_;
};

TEST_F(ClientMetadataTest, TestMakeDirs) {
}

}  // namespace vsfs
