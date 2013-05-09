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
#include <string>
#include <utility>
#include <memory>
#include "vsfs/index/range_index.h"

using ::testing::ContainerEq;
using ::testing::ElementsAre;
using std::sort;
using std::string;
using vsfs::index::RangeIndex;
using vsfs::index::RangeIndexInterface;

typedef RangeIndexInterface::FileIdType FileIdType;
typedef RangeIndexInterface::FileIdVector FileIdVector;

namespace vsfs {

TEST(RangeIndexTest, InsertToUint64KeyIndex) {
  RangeIndex<uint64_t> idx;

  FileIdVector inserted_ids = {10, 11, 12};
  uint64_t key = 4;
  for (auto id : inserted_ids) {
    idx.insert(key, id);
  }
  // Inserting duplicated records will overwrite the previous record.
  idx.insert(key, 12);

  FileIdVector file_ids;
  idx.search(key, &file_ids);
  sort(file_ids.begin(), file_ids.end());
  EXPECT_THAT(file_ids, ContainerEq(inserted_ids));

  file_ids.clear();
  idx.search(uint64_t(3), &file_ids);
  EXPECT_TRUE(file_ids.empty());
}

TEST(RangeIndexFileTest, RemoveAValueFromUint64KeyIndex) {
  RangeIndex<uint64_t> idx;

  uint64_t key = 4;
  for (uint64_t i = 0; i < 10; i++) {
    idx.insert(key, i);
  }

  // Removes all odd file ids.
  for (uint64_t i = 1; i < 10; i+=2) {
    idx.erase(key, i);
  }

  idx.erase(key, 3);

  FileIdVector file_ids;
  idx.search(key, &file_ids);
  sort(file_ids.begin(), file_ids.end());
  EXPECT_THAT(file_ids, ElementsAre(0, 2, 4, 6, 8));
}

TEST(RangeIndexFileTest, RemoveAllRecordsWithSameKey) {
  RangeIndex<uint64_t> idx;

  uint64_t key = 10;
  for (uint64_t i = 0; i < 5; i++) {
    idx.insert(key, i);
  }
  // Inserts two more unrelated records.
  idx.insert(uint64_t(8), 8);
  idx.insert(uint64_t(9), 9);

  FileIdVector file_ids;
  idx.search(key, &file_ids);
  sort(file_ids.begin(), file_ids.end());
  EXPECT_THAT(file_ids, ElementsAre(0, 1, 2, 3, 4));

  file_ids.clear();
  // Removes all records with key 10.
  idx.erase(key);
  idx.search(key, &file_ids);
  sort(file_ids.begin(), file_ids.end());
  EXPECT_TRUE(file_ids.empty());
}

TEST(RangeIndexTest, RangeSearchInFloatIndex) {
  RangeIndex<float> idx;

  FileIdVector expected_files;
  for (float i = 0.2; i < 2; i += 0.2) {
    idx.insert(i, FileIdType(i * 10));
    expected_files.push_back(i*10);
  }

  FileIdVector actual_files;
  // Performs a range query on the float index.
  float low = 0.8, high = 1.5;
  idx.search(low, false, high, false, &actual_files);
  EXPECT_THAT(actual_files, ElementsAre(8, 10, 12, 14));
}

TEST(RangeIndexTest, SplitRangeIndex) {
  // RangeIndex<uint64_t> idx;
  std::unique_ptr<RangeIndexInterface> idx(
     RangeIndexInterface::create_range_index(TypeIDs::UINT64));
  for (uint64_t i = 1; i <= 100; i++) {
    idx->insert(i, i);
  }

  // RangeIndex<uint64_t> new_idx
  std::unique_ptr<RangeIndexInterface> new_idx(
      RangeIndexInterface::create_range_index(TypeIDs::UINT64));
  std::pair<uint64_t, uint64_t> range;
  range = idx->split(new_idx.get());
  uint64_t median = range.first;
  uint64_t end = range.second;
  EXPECT_EQ(uint64_t(51), median);
  EXPECT_EQ(uint64_t(100), end);
  EXPECT_EQ(size_t(50), idx->size());
  EXPECT_EQ(size_t(50), idx->num_keys());
  EXPECT_EQ(size_t(50), new_idx->size());
  EXPECT_EQ(size_t(50), new_idx->num_keys());
}

}  // namespace vsfs
