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

#include <boost/utility.hpp>
#include <mutex>
#include <unordered_map>
#include <unordered_set>  // NOLINT
#include <vector>
#include "vobla/map_util.h"
#include "vobla/traits.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/types.h"

using std::mutex;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using vobla::find_or_null;

namespace vsfs {
namespace index {

class HashIndexInterface : boost::noncopyable {
 public:
  typedef vector<ObjectId> FileIdVector;

  static HashIndexInterface* create_hash_index(int key_type);

  HashIndexInterface() = default;

  virtual ~HashIndexInterface() {}
};

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

  void insert(KeyType key, ObjectId obj_id) {
    MutexGuard guard(lock_);
    index_[key].insert(obj_id);
  }

  void erase(KeyType key, ObjectId obj_id) {
    MutexGuard guard(lock_);
    auto obj_ids = find_or_null(index_, key);
    if (obj_ids) {
      obj_ids->erase(obj_id);
      if (obj_ids->empty()) {
        index_.erase(key);
      }
    }
  }

  void search(KeyType key, FileIdVector* results) {
    CHECK_NOTNULL(results);
    MutexGuard lock(lock_);
    auto files = find_or_null(index_, key);
    if (files) {
      results->insert(results->end(), files->begin(), files->end());
    }
  }

 private:
  typedef unordered_set<ObjectId> FileIdSet;

  typedef unordered_map<KeyType, FileIdSet> IndexHashMap;

  IndexHashMap index_;

  int key_type_;

  mutex lock_;
};

}  // namespace index
}  // namespace vsfs

#endif  // VSFS_INDEX_HASH_INDEX_H_
