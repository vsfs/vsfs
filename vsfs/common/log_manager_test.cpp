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
#include <gtest/gtest.h>
#include <fstream>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vsfs/common/log_manager.h"
#include "vsfs/rpc/vsfs_types.h"

namespace fs = boost::filesystem;
using std::ifstream;
using std::ofstream;
using std::string;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;

namespace vsfs {

typedef RpcIndexUpdate Record;

typedef LogManager<Record> TestLogManager;

class LogManagerTest : public ::testing::Test {
 protected:
  enum { BUFFER_SIZE = 1024 };

  void SetUp() {
    const char kBuffer[BUFFER_SIZE] = {};  // Random buffer
    buffer_.assign(kBuffer, BUFFER_SIZE);
    tmpdir_.reset(new TemporaryDirectory);
    testdir_ = tmpdir_->path();
  }

  void TearDown() {
    fs::remove_all(testdir_);
  }

  void append_logs(TestLogManager *manager, uint64_t num_logs) {
    for (uint64_t i = 1; i <= num_logs; ++i) {
      Record *record = new Record;
      record->txn_id = i;
      record->updates.emplace_back();
      auto& update_list = record->updates.back();
      update_list.root_path = "a";
      update_list.name = buffer_;
      manager->append(i, record);
    }
  }

  string buffer_;

  string testdir_;

  unique_ptr<TemporaryDirectory> tmpdir_;
};

TEST_F(LogManagerTest, TestAppend) {
  TestLogManager test_manager;

  for (uint64_t i = 1; i < 11; i++) {
    Record* record = new Record;
    record->txn_id = i;
    test_manager.append(i, record);
  }

  EXPECT_EQ(10u, test_manager.size());
}

TEST_F(LogManagerTest, TestEraseTo) {
  TestLogManager test_manager;
  append_logs(&test_manager, 100);
  test_manager.erase(50);
  EXPECT_EQ(50u, test_manager.size());
}

TEST_F(LogManagerTest, TestScan) {
  TestLogManager test_manager;
  append_logs(&test_manager, 100);

  vector<const Record*> records;
  EXPECT_TRUE(test_manager.scan(8, &records).ok());

  records.clear();
  EXPECT_TRUE(test_manager.scan(110, &records).ok());
  EXPECT_EQ(100u, records.size());
}

TEST_F(LogManagerTest, TestFlushRecords) {
  TestLogManager test_manager;
  append_logs(&test_manager, 100);

  string log_file = testdir_ + "/test.log";
  ofstream ofs(log_file);
  EXPECT_TRUE(test_manager.flush(4 * BUFFER_SIZE, &ofs, true).ok());
  EXPECT_EQ(4u, test_manager.last_flushed_txn_id());
  EXPECT_NEAR(4 * BUFFER_SIZE, fs::file_size(log_file), 512);
  EXPECT_TRUE(test_manager.flush(4 * BUFFER_SIZE, &ofs, true).ok());

  TestLogManager new_manager;
  ifstream ifs(log_file);
  EXPECT_TRUE(new_manager.load(&ifs).ok());
  EXPECT_EQ(8u, new_manager.size());
}

TEST_F(LogManagerTest, TestFlushMultiTimes) {
  TestLogManager test_manager;
  append_logs(&test_manager, 100);

  // Flush
  for (uint64_t i = 0; i < 10; i++) {
    string log_file = testdir_ + "/test.log." + std::to_string(i);
    ofstream ofs(log_file);
    EXPECT_TRUE(test_manager.flush(4 * BUFFER_SIZE, &ofs, true).ok());
    EXPECT_EQ(4u * (i + 1), test_manager.last_flushed_txn_id());
    EXPECT_NEAR(4 * BUFFER_SIZE, fs::file_size(log_file), 512);
  }
}

}  // namespace vsfs
