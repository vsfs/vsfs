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

#ifndef VSFS_INDEX_HASH_INDEX_H_
#define VSFS_INDEX_HASH_INDEX_H_

#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <mutex>
#include <unordered_map>
#include <unordered_set>  // NOLINT
#include <vector>
#include "vobla/map_util.h"
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/types.h"
#include "vsfs/rpc/vsfs_types.h"

using boost::lexical_cast;
using std::mutex;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using vobla::Status;
using vobla::find_or_null;

namespace vsfs {
namespace index {

/**
 * \class HashIndexInterface
 * \brief The interface for a Hash-based index, providing faster point query.
 */
class HashIndexInterface : boost::noncopyable {
 public:
  typedef vector<ObjectId> FileIdVector;

  /// Creates a HashIndex instance with the given key type.
  static HashIndexInterface* create_hash_index(int key_type);

  HashIndexInterface() = default;

  virtual ~HashIndexInterface() {}

  virtual int key_type() const = 0;

  template <typename K>
  void insert(K key, ObjectId obj_id) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    insert(&key, obj_id);
  }

  template <typename K>
  void erase(K key) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    erase(&key);
  }

  template <typename K>
  void erase(K key, ObjectId obj_id) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    erase(&key, obj_id);
  }

  template <typename K>
  void search(K key, FileIdVector *results) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    search(&key, results);
  }

  /// Apply a batch of modifications.
  virtual Status apply(const vector<RpcIndexRecordUpdateOp> &op) = 0;

 protected:
  virtual void insert(void* key, ObjectId obj_id) = 0;
  virtual void erase(void* key) = 0;
  virtual void erase(void* key, ObjectId obj_id) = 0;
  virtual void search(void* key, FileIdVector* results) = 0;
};

/**
 * \class HashIndex
 * \brief Hash-based index, providing fast point query.
 */
template <typename Key>
class HashIndex : HashIndexInterface {
 public:
  typedef Key KeyType;
  typedef vector<ObjectId> FileIdVector;

  HashIndex() : key_type_(TypeToInt<Key>::value()) {
  }

  virtual ~HashIndex() {
  }

  /// Returns true of two HashIndex both have the same key type and have the
  /// same content.
  bool operator==(const HashIndex& rhs) const {
    return key_type_ == rhs.key_type_ && index_ = rhs.index_;
  }

  /// Returns the integer representing the key type.
  int key_type() const {
    return key_type_;
  }

  void insert(KeyType key, ObjectId obj_id) {
    MutexGuard guard(lock_);
    index_[key].insert(obj_id);
  }

  void erase(KeyType key) {
    MutexGuard lock(lock_);
    index_.erase(key);
  }

  void erase(KeyType key, ObjectId obj_id) {
    MutexGuard guard(lock_);
    erase_with_lock(key, obj_id);
  }

  void search(KeyType key, FileIdVector* results) {
    CHECK_NOTNULL(results);
    MutexGuard lock(lock_);
    auto files = find_or_null(index_, key);
    if (files) {
      results->insert(results->end(), files->begin(), files->end());
    }
  }

  bool empty() {
    MutexGuard lock(lock_);
    return index_.empty();
  }

 protected:
  void insert(void* key, ObjectId obj_id) {
    Key k = *static_cast<Key*>(key);
    insert(k, obj_id);
  }

  void erase(void* key) {
    Key k = *static_cast<Key*>(key);
    erase(k);
  }

  void erase(void* key, ObjectId obj_id) {
    Key k = *static_cast<Key*>(key);
    erase(k, obj_id);
  }

  void search(void* key, FileIdVector* results) {
    Key k = *static_cast<Key*>(key);
    search(k, results);
  }

  Status apply(const vector<RpcIndexRecordUpdateOp>& updates) {
    MutexGuard guard(lock_);
    for (const auto& update : updates) {
      KeyType key = 0;
      ObjectId file_id = 0;
      try {
        key = lexical_cast<KeyType>(update.key);
        file_id = lexical_cast<ObjectId>(update.value);
      } catch(boost::bad_lexical_cast e) {
        return Status(-EINVAL, "HashIndex::update: Bad cast");
      }

      switch (update.op) {
        case RpcIndexUpdateOpCode::INSERT:
        case RpcIndexUpdateOpCode::UPDATE:
          index_[key].insert(file_id);
          break;
        case RpcIndexUpdateOpCode::REMOVE:
          erase_with_lock(key, file_id);
          break;
        default:
          LOG(ERROR) << "Unknown IndexUpdate operation: op:" << update.op;
          return Status(-EINVAL, "Unknown op.");
      }
    }
    return Status::OK;
  }

 private:
  typedef unordered_set<ObjectId> FileIdSet;

  typedef unordered_map<KeyType, FileIdSet> IndexHashMap;

  void erase_with_lock(KeyType key, ObjectId obj_id) {
    auto obj_ids = find_or_null(index_, key);
    if (obj_ids) {
      obj_ids->erase(obj_id);
      if (obj_ids->empty()) {
        index_.erase(key);
      }
    }
  }

  IndexHashMap index_;

  int key_type_;

  mutex lock_;
};

}  // namespace index
}  // namespace vsfs

#endif  // VSFS_INDEX_HASH_INDEX_H_
