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

#include <boost/call_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <mutex>
#include <unordered_map>
#include <unordered_set>  // NOLINT
#include <string>
#include <vector>
#include "vobla/map_util.h"
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/types.h"
#include "vsfs/index/index_impl.h"
#include "vsfs/rpc/vsfs_types.h"

using boost::call_traits;
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
  void insert(const K& key, ObjectId obj_id) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    insert_internal(&key, obj_id);
  }

  template <typename K>
  void erase(const K& key) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    erase_internal(&key);
  }

  template <typename K>
  void erase(const K& key, ObjectId obj_id) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    erase_internal(&key, obj_id);
  }

  /**
   * \note the K type can not be 'char*' if using HashIndex<string>. The caller
   * must pass a string as key.
   */
  template <typename K>
  void search(const K& key, FileIdVector *results) {
    CHECK_EQ(TypeToInt<K>::value(), key_type());
    search_internal(&key, results);
  }

  void search(const std::string& key, FileIdVector* results) {
    CHECK_EQ(TypeToInt<std::string>::value(), key_type());
    search_internal(&key, results);
  }

  /// Apply a batch of modifications.
  virtual Status apply(const vector<RpcIndexRecordUpdateOp> &op) = 0;

 protected:
  virtual void insert_internal(const void* key, ObjectId obj_id) = 0;
  virtual void erase_internal(const void* key) = 0;
  virtual void erase_internal(const void* key, ObjectId obj_id) = 0;
  virtual void search_internal(const void* key, FileIdVector* results) = 0;
};

/**
 * \class HashIndex
 * \brief Hash-based index, providing fast point query.
 */
template <typename Key>
class HashIndex : public HashIndexInterface {
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
    return key_type_ == rhs.key_type_ && index_ == rhs.index_;
  }

  /// Returns the integer representing the key type.
  int key_type() const {
    return key_type_;
  }

  void insert(typename call_traits<KeyType>::param_type key, ObjectId obj_id) {
    index_impl_.insert(key, obj_id);
  }

  void erase(typename call_traits<KeyType>::param_type key) {
    index_impl_.erase(key);
  }

  void erase(typename call_traits<KeyType>::param_type key, ObjectId obj_id) {
    index_impl_.erase(key, obj_id);
  }

  /*
  void search(typename call_traits<KeyType>::param_type key,
              FileIdVector* results) {
    index_impl_.search(key, results);
  }
  */

  bool empty() {
    return index_impl_.empty();
  }

 protected:
  void insert_internal(const void* key, ObjectId obj_id) {
    typename call_traits<Key>::const_reference k =
        *static_cast<const Key*>(key);
    insert(k, obj_id);
  }

  void erase_internal(const void* key) {
    typename call_traits<Key>::const_reference k =
        *static_cast<const Key*>(key);
    erase(k);
  }

  void erase_internal(const void* key, ObjectId obj_id) {
    typename call_traits<Key>::const_reference k =
        *static_cast<const Key*>(key);
    erase(k, obj_id);
  }

  void search_internal(const void* key, FileIdVector* results) {
    typename call_traits<Key>::const_reference k =
        *static_cast<const Key*>(key);
    index_impl_.search(k, results);
  }

  Status apply(const vector<RpcIndexRecordUpdateOp>& updates) {
    return index_impl_.apply(updates);
  }

 private:
  typedef unordered_set<ObjectId> FileIdSet;

  typedef unordered_map<KeyType, FileIdSet> IndexHashMap;

  /// The caller must hold the lock_.
  void erase_with_lock(typename call_traits<KeyType>::param_type key,
                       ObjectId obj_id) {
    auto obj_ids = find_or_null(index_, key);
    if (obj_ids) {
      obj_ids->erase(obj_id);
      if (obj_ids->empty()) {
        index_.erase(key);
      }
    }
  }

  internal::IndexImpl<KeyType, IndexHashMap> index_impl_;

  IndexHashMap index_;

  int key_type_;

  mutex lock_;
};

}  // namespace index
}  // namespace vsfs

#endif  // VSFS_INDEX_HASH_INDEX_H_
