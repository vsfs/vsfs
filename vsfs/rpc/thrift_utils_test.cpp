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
#include <string>
#include "vsfs/rpc/thrift_utils.h"
#include "vsfs/rpc/testdata/test_types.h"

using std::string;

namespace vsfs {
namespace rpc {

TEST(ThriftUtilsTest, TestSerialization) {
  testing::Directory directory;
  directory.name = "test";
  directory.subdirs.insert("sub0");
  directory.subdirs.insert("sub1");
  directory.subdirs.insert("sub2");

  string content = ThriftUtils::serialize(directory);

  testing::Directory other;
  ThriftUtils::deserialize(content, &other);
  EXPECT_EQ(directory, other);
}

TEST(ThriftUtilsTest, TestDeserializeFailures) {
  testing::Directory test_obj;
  EXPECT_FALSE(ThriftUtils::deserialize("", &test_obj));
}

}  // namespace rpc
}  // namespace vsfs
