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

/**
 * \file test_leveldb_store.h
 * \brief A in-ram "LevelDB" used for testing purpose.
 */

#ifndef VSFS_COMMON_TEST_LEVELDB_STORE_H_
#define VSFS_COMMON_TEST_LEVELDB_STORE_H_

#include <map>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/leveldb_store.h"

using std::map;
using std::string;
using vobla::Status;

namespace vsfs {

/**
 * \brief A testing LevelDB that keep all data in RAM. It should only be used
 * for testing purpose.
 *
 * The difference between using TestLevelDBStore and using MockLevelDBStore is
 * that this class can be a drop-in replacement for LevelDBStore, without
 * checking the method calls. In other word, this class is simpler than
 * MockLevelDBStore to be used in most cases. However, it can not control the
 * behavior of LevelDBStore, e.g., simulate the disk failures of LevelDB, in
 * which case it is preferable to use MockLevelDBStore.
 */
class TestLevelDBStore : public LevelDBStore {
 public:
  /// Constructs a test leveldb store in RAM.
  TestLevelDBStore() = default;

  /// Constructs a test leveldb store with initialized values.
  TestLevelDBStore(const map<string, string>& other);

  ~TestLevelDBStore() = default;

  /// Open a testing LevelDB, always success.
  Status open() { return Status::OK; }

  /// Creates a testing LevelDB, always success.
  Status create() { return Status::OK; }

  Status get(const string& key, string* value);

  Status put(const string& key, const string &value);

  Status remove(const string& key);

  iterator begin();

  iterator end();

  const map<string, string>& store() const;

 private:
  map<string, string> store_;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_TEST_LEVELDB_STORE_H_
