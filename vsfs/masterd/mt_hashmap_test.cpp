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
#include <set>
#include <string>
#include "vsfs/masterd/mt_hashmap.h"

using std::string;
using std::set;

namespace vsfs {
namespace masterd {

struct NameSet {
  set<string> names;
};
typedef MTHashMap<string, NameSet> HashMap;

TEST(MTHashMap, TestConstructor) {
  HashMap test_map;
  EXPECT_TRUE(test_map.empty());
  EXPECT_EQ(0, test_map.size());
  EXPECT_EQ(1024, test_map.bucket_size());
  EXPECT_THROW(test_map.at("123"), std::out_of_range);
}

}  // namespace masterd
}  // namespace vsfs
