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
#include <gflags/gflags.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vobla/traits.h"
#include "vsfs/index/index_info.h"
#include "vsfs/index/range_index.h"
#include "vsfs/indexd/index_manager.h"
#include "vsfs/rpc/vsfs_types.h"

namespace fs = boost::filesystem;
using ::testing::ElementsAre;
using std::string;
using std::unique_ptr;
using std::vector;
using vsfs::index::IndexInfo;
using vsfs::index::RangeIndex;
using vsfs::index::RangeIndexInterface;

DECLARE_bool(update_immediately);

namespace vsfs {
namespace indexd {

static char kRamTestDir[] = "";

class IndexManagerTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    old_update_immediately_ = FLAGS_update_immediately;
    tmpdir_.reset(new vobla::TemporaryDirectory);
    testdir_ = tmpdir_->path();
  }

  virtual void TearDown() {
    FLAGS_update_immediately = old_update_immediately_;
  }

  Status update_index(IndexManager* manager,
                      int64_t txn_id, const string &path, const string &name,
                      int num_records) {
    CHECK_NOTNULL(manager);
    RpcIndexUpdate updates;
    updates.txn_id = txn_id;
    updates.updates.emplace_back();
    vector<RpcIndexRecordUpdateOp> record_updates;
    updates.updates[0].root_path = path;
    updates.updates[0].name = name;
    for (int i = 0; i < num_records; ++i) {
      updates.updates[0].record_updates.emplace_back();
      updates.updates[0].record_updates[i].op = RpcIndexUpdateOpCode::INSERT;
      updates.updates[0].record_updates[i].key = lexical_cast<string>(i);
      updates.updates[0].record_updates[i].value = lexical_cast<string>(i);
    }
    return manager->update(&updates);
  }

  unique_ptr<vobla::TemporaryDirectory> tmpdir_;

  string testdir_;

  bool old_update_immediately_;
};

TEST_F(IndexManagerTest, TestCreateRangeIndex) {
  IndexManager manager(testdir_);
  Status status;
  status = manager.create(0, "/home/test", "test-index",
                          IndexInfo::BTREE, TypeIDs::UINT64);
  EXPECT_TRUE(status.ok());

  RangeIndexInterface* index = manager.get_range_index("/home/test",
                                                       "test-index");
  ASSERT_TRUE(index != NULL);
  EXPECT_EQ(TypeIDs::UINT64, index->key_type());
}

TEST_F(IndexManagerTest, TestCreateFailures) {
  IndexManager manager(kRamTestDir);

  // not valid path and names.
  EXPECT_FALSE(manager.create(0, "", "index",
                              IndexInfo::BTREE, TypeIDs::FLOAT).ok());
  EXPECT_FALSE(manager.create(0, "/abc", "",
                              IndexInfo::BTREE, TypeIDs::FLOAT).ok());

  // Unsupported index types.
  EXPECT_FALSE(manager.create(0, "/abc", "index",
                              IndexInfo::HASH, TypeIDs::FLOAT).ok());
  EXPECT_FALSE(manager.create(0, "/abc", "index",
                              IndexInfo::UNKNOWN_INDEX, TypeIDs::FLOAT).ok());
  EXPECT_FALSE(manager.create(0, "/abc", "index",
                              10000, TypeIDs::FLOAT).ok());

  manager.create(0, "/home/test", "test-index",
                 IndexInfo::BTREE, TypeIDs::UINT64);
  // The name is being taken.
  EXPECT_FALSE(manager.create(0, "/home/test", "test-index",
                              IndexInfo::BTREE, TypeIDs::FLOAT).ok());
}

TEST_F(IndexManagerTest, TestUpdateBasics) {
  // The basic/simplest cases to update an index.
  FLAGS_update_immediately = true;
  IndexManager manager(testdir_);
  manager.create(0, "/path", "index", IndexInfo::BTREE, TypeIDs::UINT64);
  EXPECT_TRUE(update_index(&manager, 100, "/path", "index", 10).ok());

  RangeIndexInterface* index = manager.get_range_index("/path", "index");
  EXPECT_TRUE(index != nullptr);
  EXPECT_EQ(static_cast<size_t>(10), index->size());
  EXPECT_EQ(static_cast<size_t>(10), index->num_keys());
}

TEST_F(IndexManagerTest, TestUpdateOnlyAppendLogs) {
  IndexManager manager(kRamTestDir);
  manager.create(0, "/path", "index", IndexInfo::BTREE, TypeIDs::UINT64);
  for (uint64_t txn_id = 100; txn_id < 110; txn_id++) {
    RpcIndexUpdate updates;
    updates.txn_id = txn_id;
    updates.updates.emplace_back();
    vector<RpcIndexRecordUpdateOp> record_updates;
    updates.updates[0].root_path = "/path";
    updates.updates[0].record_updates.emplace_back();
    updates.updates[0].record_updates[0].op = RpcIndexUpdateOpCode::INSERT;
    updates.updates[0].record_updates[0].key = lexical_cast<string>(txn_id);
    updates.updates[0].record_updates[0].value = lexical_cast<string>(txn_id);
    EXPECT_TRUE(manager.update(&updates).ok());
  }
  RangeIndexInterface* index = manager.get_range_index("/path", "index");
  EXPECT_EQ(0u, index->size());

  // Force merge
  EXPECT_TRUE(manager.range_index_map_["/path"]->merge(104).ok());
  EXPECT_EQ(5u, index->size());
}

/*
TEST_F(IndexManagerTest, TestSearchBasic) {
  FLAGS_update_immediately = true;
  vector<bool> flags = { true, false };
  for (auto flag : flags) {
    FLAGS_update_immediately = flag;
    IndexManager manager(testdir_);
    manager.create(0, "/foo/bar", "index", IndexInfo::BTREE, TypeIDs::INT32);

    RpcIndexUpdate updates;
    updates.txn_id = 100;
    updates.updates.emplace_back();
    vector<RpcIndexRecordUpdateOp> record_updates;
    updates.updates[0].root_path = "/foo/bar";
    updates.updates[0].name = "index";
    for (size_t i = 0; i < 10; ++i) {
      updates.updates[0].record_updates.emplace_back();
      updates.updates[0].record_updates[i].op = RpcIndexUpdateOpCode::INSERT;
      updates.updates[0].record_updates[i].key = lexical_cast<string>(10 + i);
      updates.updates[0].record_updates[i].value =
          lexical_cast<string>(100 + i);
    }
    EXPECT_TRUE(manager.update(&updates).ok());

    RpcComplexQuery query;
    query.txn_id = 102;
    query.path_prefix = "/foo/bar";
    query.range_queries.emplace_back();
    auto& rq = query.range_queries.back();
    rq.index_path = "/foo/bar";
    rq.name = "index";
    rq.lower = "12";
    rq.lower_open = false;
    rq.upper = "17";
    rq.upper_open = false;

    vector<int64_t> file_ids;
    EXPECT_TRUE(manager.search(query, &file_ids).ok());
    EXPECT_THAT(file_ids, ElementsAre(102, 103, 104, 105, 106, 107));
  }
}
*/

TEST_F(IndexManagerTest, TestFlush) {
  IndexManager manager(testdir_);
  manager.create(0, "/foo/bar/.vsfs/index.index", "index",
                 IndexInfo::BTREE, TypeIDs::INT32);
  manager.create(0, "/foo/bar/.vsfs/energy.index", "energy",
                 IndexInfo::BTREE, TypeIDs::INT32);
  manager.create(0, "/foo/zoo/.vsfs/index.index", "index",
                 IndexInfo::BTREE, TypeIDs::INT32);

  EXPECT_TRUE(manager.flush().ok());

  /*
  fs::directory_iterator end;
  int num_indices = 0;
  for (fs::directory_iterator it(testdir_); it != end; ++it) {
    LOG(INFO) << it->path();
    fs::path ext = it->path().extension();
    if (ext == ".idx") {
      num_indices++;
    }
  }
  EXPECT_EQ(3, num_indices);
  */
}

TEST_F(IndexManagerTest, TestMergeForAnsweringInfoRequest) {
  IndexManager manager(testdir_);
  manager.create(0, "/path", "index",
                 IndexInfo::BTREE, TypeIDs::UINT64);
  EXPECT_TRUE(update_index(&manager, 100, "/path", "index", 100).ok());

  RpcIndexInfoRequest request;
  request.txn_id = 101;
  request.path = "/path";
  request.name = "index";

  RpcIndexInfo info;
  EXPECT_TRUE(manager.info(request, &info).ok());
  EXPECT_EQ(100u, info.num_records);
}

class RangeIndexWrapperTest : public ::testing::Test {
 protected:
  typedef RangeIndex<uint64_t> RangeIndexType;

  void SetUp() {
    tmpdir_.reset(new vobla::TemporaryDirectory);
    testdir_ = tmpdir_->path();
  }

  unique_ptr<vobla::TemporaryDirectory> tmpdir_;

  string testdir_;
};

TEST_F(RangeIndexWrapperTest, TestFlushLog) {
  const string root = "/foo/bar";
  const string name = "test";

  RangeIndexType *index = new RangeIndexType;
  IndexManager::RangeIndexWrapper wrapper(testdir_ + "/logfile", index);

  for (uint64_t txn_id = 1; txn_id <= 10; ++txn_id) {
    RpcIndexRecordUpdateList updates;
    updates.root_path = root;
    updates.name = name;

    wrapper.update(txn_id, &updates);
  }

  const string log_file = testdir_ + "/logfile.log";
  EXPECT_TRUE(wrapper.flush_log(true).ok());
  EXPECT_LE(10 * (root.size() + name.size()), fs::file_size(log_file));
}

TEST_F(RangeIndexWrapperTest, TestDisallowMergeWhenMigrate) {
  RangeIndexType *index = new RangeIndexType;
  IndexManager::RangeIndexWrapper wrapper(testdir_ + "/logfile", index);

  const string root = "/foo/bar";
  const string name = "test";

  for (uint64_t txn_id = 1; txn_id <= 10; ++txn_id) {
    RpcIndexRecordUpdateList updates;
    updates.root_path = root;
    updates.name = name;

    wrapper.update(txn_id, &updates);
  }

  EXPECT_EQ(IndexManager::RangeIndexWrapper::NORMAL, wrapper.status());
  wrapper.set_status(IndexManager::RangeIndexWrapper::MIGRATING);
  EXPECT_EQ(IndexManager::RangeIndexWrapper::MIGRATING, wrapper.status());

  Status status;
  status = wrapper.merge(20);
  EXPECT_FALSE(status.ok());

  wrapper.set_status(IndexManager::RangeIndexWrapper::NORMAL);
  EXPECT_TRUE(wrapper.merge(21).ok());
}

}  // namespace indexd
}  // namespace vsfs
