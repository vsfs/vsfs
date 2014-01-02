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

#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>
#include "vsfs/common/leveldb_store.h"

using ::testing::ElementsAre;
using std::string;
using std::to_string;
using std::vector;
namespace fs = boost::filesystem;

namespace vsfs {

class LevelDBStoreTest : public ::testing::Test {
 protected:
  void SetUp() {
    testdir_ = (fs::temp_directory_path() / fs::unique_path()).string();
    fs::create_directories(testdir_);
  }

  void TearDown() {
    fs::remove_all(testdir_);
  }

  string testdir_;
};

TEST_F(LevelDBStoreTest, CreateAndOpen) {
  LevelDBStore leveldb(testdir_ + "/db1");
  EXPECT_TRUE(leveldb.create().ok());

  EXPECT_TRUE(leveldb.put("1", "a").ok());
  EXPECT_TRUE(leveldb.put("2", "b").ok());

  string value;
  EXPECT_TRUE(leveldb.get("2", &value).ok());
  EXPECT_EQ(string("b"), value);

  // Value is a buffer, not a C-string.
  string value3("ab\0cde", 6);
  EXPECT_TRUE(leveldb.put("3", value3).ok());
  EXPECT_TRUE(leveldb.get("3", &value).ok());
  EXPECT_EQ(6u, value.size());
  EXPECT_EQ(value3, value);
}

TEST_F(LevelDBStoreTest, OpenAnExistedDb) {
  // Inserts 10 elements to a new created db and closes it.
  {
    LevelDBStore db(testdir_ + "/db2");
    db.create();
    for (int i = 0; i < 10; ++i) {
      EXPECT_TRUE(db.put(to_string(i),
                         to_string(i)).ok());
    }
  }

  // Re-opens the db and check the existence of records.
  LevelDBStore db(testdir_ + "/db2");
  EXPECT_TRUE(db.open().ok());
  for (int i = 0; i < 10; i++) {
    string k, v;
    k = to_string(i);
    EXPECT_TRUE(db.get(k, &v).ok());
    EXPECT_EQ(k, v);
  }

  int i = 0;
  for (const auto& key_and_value : db) {
    EXPECT_EQ(key_and_value.first, key_and_value.second);
    EXPECT_EQ(to_string(i), key_and_value.first);
    i++;
  }
}

TEST_F(LevelDBStoreTest, TestSearchByPrefix) {
  LevelDBStore db(testdir_ + "/search1");
  db.create();
  db.put("abcdefg", "abcdefg");
  db.put("aaaaaa", "aaaaa");
  db.put("aaabbb", "aabbb");
  db.put("bcdeef", "bcdeef");

  CHECK(db.search("c") == db.end());
  vector<string> values;
  for (auto it = db.search("a"); it.starts_with("a"); ++it) {
    values.push_back(it->second);
  }
  std::sort(values.begin(), values.end());
  EXPECT_THAT(values, ElementsAre("aaaaa", "aabbb", "abcdefg"));

  values.clear();
  for (auto it = db.search("ab"); it != db.end(); ++it) {
    values.push_back(it->second);
  }
  EXPECT_THAT(values, ElementsAre("abcdefg", "bcdeef"));
}

}  // namespace vsfs
