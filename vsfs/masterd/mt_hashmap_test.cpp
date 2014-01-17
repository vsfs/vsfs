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
#include <set>
#include <string>
#include <utility>
#include "vsfs/masterd/mt_hashmap.h"

using ::testing::ElementsAre;
using std::make_pair;
using std::set;
using std::string;

namespace vsfs {
namespace masterd {

struct NameSet {
  set<string> names;
};

typedef MTHashMap<string, NameSet> HashMap;
typedef MTHashMap<string, string> StringHashMap;

TEST(MTHashMap, TestConstructor) {
  HashMap test_map;
  EXPECT_TRUE(test_map.empty());
  EXPECT_EQ(0, test_map.size());
  EXPECT_EQ(1024, test_map.bucket_size());
  EXPECT_THROW(test_map.at("123"), std::out_of_range);
}

TEST(MTHashMap, TestInsert) {
  HashMap test_map;
  NameSet name_set;
  name_set.names = {"1", "2", "3"};
  auto res = test_map.insert(make_pair(string("abc"), name_set));
  EXPECT_TRUE(res.second);
  EXPECT_EQ("abc", res.first->first);
  EXPECT_THAT(res.first->second.names, ElementsAre("1", "2", "3"));

  auto it = test_map.begin();
  EXPECT_EQ(it, res.first);
  ++it;
  EXPECT_THAT(it, test_map.end());
}

TEST(MTHashMap, TestAssignAndEqual) {
  StringHashMap test_map;
  test_map["1"] = "1";
  test_map["2"] = "2";
  test_map["3"] = "3";
  StringHashMap new_map = test_map;
  EXPECT_EQ(3, new_map.size());
  EXPECT_TRUE(test_map == new_map);
  EXPECT_EQ(test_map, new_map);
}

TEST(MTHashMap, TestIteration) {
  StringHashMap test_map;
  test_map["1"] = "1";
  test_map["2"] = "2";
  test_map["3"] = "3";
  test_map["4"] = "4";
  EXPECT_EQ(4, test_map.size());

  set<string> values;
  for (auto it = test_map.begin(); it != test_map.end(); ++it) {
    values.insert(it->second);
  }
  EXPECT_THAT(values, ElementsAre("1", "2", "3", "4"));

  test_map.clear();
  EXPECT_EQ(0, test_map.size());
  EXPECT_TRUE(test_map.empty());
  EXPECT_EQ(test_map.begin(), test_map.end());
}

}  // namespace masterd
}  // namespace vsfs
