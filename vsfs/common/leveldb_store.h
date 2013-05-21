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

#ifndef VSFS_COMMON_LEVELDB_STORE_H_
#define VSFS_COMMON_LEVELDB_STORE_H_

#include <boost/iterator/iterator_facade.hpp>
#include <leveldb/db.h>
#include <memory>
#include <string>
#include <utility>
#include "vobla/macros.h"
#include "vobla/status.h"
#include "vsfs/common/key_value_store.h"

using std::unique_ptr;
using std::string;
using vobla::Status;

namespace leveldb {
class DB;
class Status;
}

namespace vsfs {

/**
 * \class LevelDBStore "vsfs/common/leveldb_store.h"
 * \brief A Key-Value persistent storage based on LevelDB.
 *
 * You can access the data through the typical K-V DB operations:
 *  - put()
 *  - get()
 *  - remove()
 */
class LevelDBStore  : public KeyValueStore {
  typedef std::pair<string, string> KeyValuePair;

 public:
  class LevelDBStoreIterator : public boost::iterator_facade<
    LevelDBStoreIterator, KeyValuePair, boost::bidirectional_traversal_tag> {
   public:
    LevelDBStoreIterator();

    explicit LevelDBStoreIterator(leveldb::Iterator* iter);

    /// ++i
    void increment();

    /// --i
    void decrement();

    /**
     * \brief Returns the reference to the key and value.
     *
     * \note In the current implementation, each time calling this function,
     * a new pair of {key, value} string values are created. It might not be
     * efficient.
     */
    reference dereference() const;

    /// it != end()?
    bool equal(LevelDBStoreIterator const& other) const;

   private:
    unique_ptr<leveldb::Iterator> iter_;

    /// A local copy of key and value on the current iterator position.
    value_type key_and_value_;
  };

  typedef LevelDBStoreIterator iterator;

  /// Constructs a
  explicit LevelDBStore(const string &path);

  ~LevelDBStore() = default;

  /// Open an existing store.
  Status open();

  /// Creates a LevelDBStore if it does not exist on disk.
  Status create();

  /// Gets a value buffer with the given key.
  Status get(const string& key, string* value);

  /// Puts a key-value pair to the leveldb.
  Status put(const string& key, const string &value);

  /// Returns a key-value pair.
  Status remove(const string& key);

  /**
   * \brief Returns an iterator referring to the first element in the DB.
   *
   * \note The iterator is for read-only purpose. It does not support change
   * the value in the DB. You should only use the iterator for the sake of
   * scanning the store.
   */
  iterator begin();

  /// Returns an iterator rerfering to the past-the-end element in the DB.
  iterator end();

 private:
  string db_path_;

  unique_ptr<leveldb::DB> db_;

  Status to_status(const leveldb::Status& l_status) const;

  DISALLOW_IMPLICIT_CONSTRUCTORS(LevelDBStore);
};

}  // namespace vsfs

#endif  // VSFS_COMMON_LEVELDB_STORE_H_
