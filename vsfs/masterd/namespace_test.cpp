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

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "vsfs/common/test_leveldb_store.h"
#include "vsfs/masterd/namespace.h"

using std::string;
using std::unique_ptr;

namespace vsfs {
namespace masterd {

/**
 * \class NamespaceTest
 * \brief Unit tests for the unified namespace.
 */
class NamespaceTest : public ::testing::Test {
 protected:
  void SetUp() {
    auto test_store = new TestLevelDBStore;
    test_ns_.reset(new Namespace(test_store));
  }

  unique_ptr<Namespace> test_ns_;
};

TEST_F(NamespaceTest, CreateFile) {
  ObjectId obj;
  string path = "/foo/bar";
  EXPECT_TRUE(test_ns_->create(path, 0x666, 100, 100, &obj).ok());

  ObjectId actual_obj;
  EXPECT_TRUE(test_ns_->file_id(path, &actual_obj).ok());
  EXPECT_EQ(obj, actual_obj);

  string actual_path;
  EXPECT_TRUE(test_ns_->file_path(obj, &actual_path).ok());
  EXPECT_EQ(path, actual_path);

  EXPECT_FALSE(test_ns_->create(path, 0x666, 100, 200, &obj).ok());
}

TEST_F(NamespaceTest, TestMakeDirs) {
  EXPECT_TRUE(test_ns_->mkdir("/", 0x666, 100, 100).ok());
  EXPECT_TRUE(test_ns_->mkdir("/foo/bar", 0x666, 100, 100).ok());
}

}  // namespace masterd
}  // namespace vsfs
