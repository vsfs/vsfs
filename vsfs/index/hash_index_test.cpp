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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include "vsfs/index/hash_index.h"

using ::testing::ContainerEq;
using std::sort;

namespace vsfs {
namespace index {

typedef HashIndexInterface::FileIdVector FileIdVector;

TEST(HashIndexTest, TestInsertUint64) {
  HashIndex<int> idx;

  FileIdVector ids = {10, 11, 12};
  for (auto id : ids) {
    idx.insert(4, id);
  }

  FileIdVector actual_ids;
  idx.search(4, &actual_ids);
  sort(actual_ids.begin(), actual_ids.end());
  EXPECT_THAT(actual_ids, ContainerEq(ids));

  actual_ids.clear();
  idx.search(5, &actual_ids);
  EXPECT_EQ(0, actual_ids.size());
}

}  // namespace index
}  // namespace vsfs
