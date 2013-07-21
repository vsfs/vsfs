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

#include <gtest/gtest.h>
#include <memory>
#include "vsfs/common/test_leveldb_store.h"
#include "vsfs/masterd/namespace.h"

using std::unique_ptr;

namespace vsfs {
namespace masterd {

class NamespaceTest : public ::testing::Test {
 protected:
  void SetUp() {
    auto test_store = new TestLevelDBStore;
    test_ns_.reset(new Namespace(test_store));
  }

  void TearDown() {
  }

  unique_ptr<Namespace> test_ns_;
};

TEST_F(NamespaceTest, TestMakeDirs) {
}

}  // namespace masterd
}  // namespace vsfs
