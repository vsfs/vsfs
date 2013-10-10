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

#ifndef VSFS_INDEX_INDEX_IMPL_H_
#define VSFS_INDEX_INDEX_IMPL_H_

#include <boost/call_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <glog/logging.h>
#include <mutex>
#include <string>
#include <unordered_set>  // NOLINT
#include <vector>
#include "vobla/status.h"
#include "vobla/map_util.h"
#include "vsfs/common/types.h"
#include "vsfs/rpc/vsfs_types.h"

using boost::call_traits;
using boost::lexical_cast;
using std::string;
using std::unordered_set;
using vobla::Status;
using vobla::find_or_null;

namespace vsfs {
namespace index {
namespace internal {

/**
 * \template IndexImpl
 * \brief Implement some basic functions for all index.
 */
template <typename Key, typename Index>
class IndexImpl : boost::noncopyable {
 public:
  typedef Key KeyType;
  typedef Index IndexType;
  typedef vector<ObjectId> FileIdVector;

  IndexImpl() {}

  bool operator==(const IndexImpl& rhs) {
    return index_ == rhs.index_;
  }

  /// Inserts one file with key.
  void insert(typename call_traits<KeyType>::param_type key,
              ObjectId obj_id) {
    MutexGuard guard(lock_);
    index_[key].insert(obj_id);
  }

  Status insert_string_val(const string& key, const string& value) {
    KeyType insert_key = 0;
    ObjectId obj_id = 0;
    try {
      insert_key = lexical_cast<KeyType>(key);
      obj_id = lexical_cast<ObjectId>(value);
    } catch(boost::bad_lexical_cast e) {
      LOG(ERROR) << "Failed to cast key: " << key << " value: " << value;
      return Status(-EINVAL, "IndexImpl::insert_string_val: bad cast");
    }
    insert(insert_key, obj_id);
    return Status::OK;
  }

  /// Applies a batch of updates to the index.
  Status apply(const vector<RpcIndexRecordUpdateOp>& updates) {
    MutexGuard guard(lock_);
    for (const auto& update : updates) {
      KeyType key = 0;
      ObjectId obj_id = 0;
      try {
        key = lexical_cast<KeyType>(update.key);
        obj_id = lexical_cast<ObjectId>(update.value);
      } catch(boost::bad_lexical_cast e) {
        return Status(-EINVAL, "IndexImpl::apply: bad cast.");
      }

      switch (update.op) {
        case RpcIndexUpdateOpCode::INSERT:
        case RpcIndexUpdateOpCode::UPDATE:
          index_[key].insert(obj_id);
          break;
        case RpcIndexUpdateOpCode::REMOVE:
          erase_with_lock(key, obj_id);
          break;
        default:
          LOG(ERROR) << "Unknown IndexUpdate operation: op:" << update.op;
          return Status(-EINVAL, "Unknown op.");
      }
    }
    return Status::OK;
  }

  /// Erases a record with given key and its obj id.
  void erase(typename call_traits<KeyType>::param_type key, ObjectId obj_id) {
    MutexGuard guard(lock_);
    erase_with_lock(key, obj_id);
  }

  void erase(typename call_traits<KeyType>::param_type key) {
    MutexGuard guard(lock_);
    index_.erase(key);
  }

  Status erase_string_val(const string &key, const string &value) {
    KeyType erase_key = 0;
    ObjectId erase_value = 0;
    try {
      erase_key = lexical_cast<KeyType>(key);
    } catch(boost::bad_lexical_cast e) {
      return Status(-EINVAL, "IndexImpl::erase_string_val: bad cast");
    }
    if (value.empty()) {
      erase(erase_key);
    } else {
      try {
        erase_value = lexical_cast<ObjectId>(value);
      } catch(boost::bad_lexical_cast e) {
        return Status(-EINVAL, "IndexImpl::erase_string_val: bad cast");
      }
      erase(erase_key, erase_value);
    }
    return Status::OK;
  }

  void clear() {
    MutexGuard guard(lock_);
    index_.clear();
  }

  bool empty() {
    MutexGuard guard(lock_);
    return index_.empty();
  }

  size_t num_keys() {
    MutexGuard guard(lock_);
    return index_.size();
  }

  /// Returns the number of records.
  size_t size() {
    size_t ret = 0;
    MutexGuard guard(lock_);
    for (const auto& iter : index_) {
      ret += iter.second.size();
    }
    return ret;
  }

  void search(typename call_traits<KeyType>::param_type key,
              FileIdVector* results) {
    CHECK_NOTNULL(results);
    MutexGuard guard(lock_);
    auto files = find_or_null(index_, key);
    if (files) {
      results->insert(results->end(), files->begin(), files->end());
    }
  }

  /// Access the underlying index.
  Index* index() {
    return &index_;
  }

  /// Access the underlying lock.
  mutex* lock() { return &lock_; }

  /**
   * \brief Splits this index based on the value (ObjectId) range.
   *
   * It first finds the median of the value (ObjectId) range, and separates
   * this index into two value ranges : [start, median), [median, end]
   *
   * \param[out] other another empty index to migrate the values from [median,
   * end] to.
   * \return Returns the median value (ObjectId).
   * \pre The 'other' index must be empty.
   */
  ObjectId split(IndexImpl* other) {
    CHECK_NOTNULL(other);
    CHECK(other->empty());
    size_t total_size = this->size();
    vector<ObjectId> all_obj_ids;
    all_obj_ids.reserve(total_size);
    MutexGuard guard(lock_);
    for (const auto& iter : index_) {
      for (const auto& file_id : iter.second) {
        all_obj_ids.push_back(file_id);
      }
    }
    size_t median_pos = total_size / 2;
    std::nth_element(all_obj_ids.begin(), all_obj_ids.begin() + median_pos,
                     all_obj_ids.end());
    ObjectId median = all_obj_ids[median_pos];
    for (const auto& iter : index_) {
      unordered_set<ObjectId> moved_obj_ids;
      for (const auto& obj_id : iter.second) {
        if (obj_id >= median) {
          moved_obj_ids.insert(obj_id);
        }
      }
      if (!moved_obj_ids.empty()) {
        other->index_[iter.first].swap(moved_obj_ids);
      }
    }
    for (const auto& iter : *other->index_) {
      const auto& key = iter.first;
      for (const auto& obj_id : iter.second) {
        index_[key].erase(obj_id);
      }
      if (index_[key].empty()) {
        index_.erase(key);
      }
    }
    return median;
  }

 private:
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

  Index index_;
  mutex lock_;
};

}  // namespace internal
}  // namespace index
}  // namespace vsfs

#endif  // VSFS_INDEX_INDEX_IMPL_H_
