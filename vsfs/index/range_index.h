/*
 * Copyright 2012-2013 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_INDEX_RANGE_INDEX_H_
#define VSFS_INDEX_RANGE_INDEX_H_

#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <algorithm>
#include <limits>
#include <map>
#include <mutex>
#include <string>
#include <unordered_set>  // NOLINT
#include <utility>
#include <vector>
#include "vobla/macros.h"
#include "vobla/map_util.h"
#include "vobla/range.h"
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/common/thread.h"
#include "vsfs/rpc/vsfs_types.h"

using boost::lexical_cast;
using std::map;
using std::mutex;
using std::string;
using std::stringstream;
using std::unordered_set;
using std::vector;
using vobla::Status;
using vobla::find_or_null;

namespace vsfs {
namespace index {

template <typename T>
class RangeIndex;

/**
 * \class RangeIndexInterface
 * \brief The interface of in-memory RangeIndex.
 */
class RangeIndexInterface {
 public:
  typedef uint64_t FileIdType;
  // TODO(ziling): use vsfs/common/range.h to replace this.
  typedef std::pair<FileIdType, FileIdType> FileIdRangeType;
  typedef FileIdType value_type;
  typedef vector<FileIdType> FileIdVector;

  /// Factory method.
  static RangeIndexInterface* create_range_index(int key_type);

  RangeIndexInterface() = default;

  virtual ~RangeIndexInterface() {}

  template <typename K>
  void insert(K key, FileIdType file_id) {
    insert(&key, sizeof(key), file_id);
  }

  // virtual Status update(const vector<RpcIndexRecordUpdateOp> &op) = 0;

  FileIdRangeType split(RangeIndexInterface* other) {
    return split_index(other);
  }

  /**
   * \brief Inserts key-value pair from a string format.
   * It first casts the string to the appropriated value, then inserts the
   * record.
   *
   * \return Status(-EINVAL, "...") if either key or file_id can not be cast
   * to appropriated value.
   */
  virtual Status insert_string_val(const string &key,
                                   const string &file_id) = 0;

  template <typename K>
  void erase(K key, FileIdType file_id) {
    erase(&key, sizeof(key), file_id);
  }

  template <typename K>
  void erase(K key) {
    erase(&key, sizeof(key));
  }

  /**
   * \brief Lexically casts the string key-value pair and erases the record(s).
   *
   * If 'value' is empty, erases all records with the same key, otherwise, only
   * erases the key-value pair.
   */
  virtual Status erase_string_val(const string &key, const string &value) = 0;

  /**
   * \brief A general interface to apply modifications.
   */
  virtual Status apply(const vector<RpcIndexRecordUpdateOp> &op) = 0;

  template <typename K>
  void search(K key, FileIdVector *results) {
    search(&key, sizeof(key), results);
  }

  template <typename K>
  void search(K lower, K upper, FileIdVector *results) {
    search(&lower, &upper, sizeof(lower), results);
  }

  template <typename K>
  void search(const vobla::Range<K>& range, FileIdVector *results);

  // virtual Status search(const RpcRangeQuery &query, FileIdVector *results) = 0; // NOLINT

  virtual int key_type() const = 0;

  virtual size_t size() = 0;

  virtual size_t num_keys() = 0;

  virtual size_t approx_bytes() = 0;

 protected:
  // TODO(eddyxu): pass the Type2Int::value to check the types.
  virtual void insert(void *key, size_t key_size, FileIdType file_id) = 0;

  virtual void erase(void *key, size_t key_size) = 0;

  virtual void erase(void *key, size_t key_size, FileIdType file_id) = 0;

  virtual void search(void *lower, void *upper, size_t key_size,
                      FileIdVector *results) = 0;

  virtual void search(void *key, size_t key_size, FileIdVector *results) = 0;

  virtual FileIdRangeType split_index(RangeIndexInterface* other) = 0;
};


/**
 * \class RangeIndex range_index.h "vsfs/index/range_index.h"
 * \tparam K key type. Must be a numeric type.
 * \brief A one-dimentional in-memory numeric index.
 *
 * Conceptually, it is a mapping from key to [file0, file1, ...]:
 * E.g.:
 *  { 2.0 --> [file0, file3, file4, ...],
 *    2.7 --> [file7, file8, file9, ...],
 *    ... }
 *
 * The key of this range index can be (signed/unsigned) integers or
 * float/double numbers.
 *
 * It supports either searching files by the key, or a key range.
 */
template <typename Key>
class RangeIndex : public RangeIndexInterface {
 public:
  typedef Key KeyType;

  RangeIndex() : key_type_(TypeToInt<Key>::value()) {
  }

  virtual ~RangeIndex() {
  }

  /// Returns true if two indices are exactly the same.
  bool operator==(const RangeIndex &rhs) const {
    return key_type_ == rhs.key_type_ && index_ == rhs.index_;
  }

  void insert(KeyType key, FileIdType file_id) {
    MutexGuard guard(lock_);
    index_[key].insert(file_id);
  }

  Status insert_string_val(const string &key, const string &value) {
    KeyType insert_key = 0;
    FileIdType file_id = 0;
    try {
      insert_key = lexical_cast<KeyType>(key);
      file_id = lexical_cast<FileIdType>(value);
    } catch(boost::bad_lexical_cast e) {
      return Status(-EINVAL, "RangeIndex::insert_string_val: Bad cast");
    }
    insert(insert_key, file_id);
    return Status::OK;
  }

  Status apply(const vector<RpcIndexRecordUpdateOp> &updates) {
    MutexGuard guard(lock_);
    for (const auto& update : updates) {
      KeyType key = 0;
      FileIdType file_id = 0;
      try {
        key = lexical_cast<KeyType>(update.key);
        file_id = lexical_cast<FileIdType>(update.value);
      } catch(boost::bad_lexical_cast e) {
        return Status(-EINVAL, "RangeIndex::update: Bad cast");
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

  /**
   * \brief Erases Key-FileId pair from the in-memory RangeIndex.
   * \tparam K the key type (e.g., uint64, int, float).
   * \param key the key to be erased.
   * \param file_id the value to be erased.
   *
   * It only erases the key-value pairs that are exactly same to the
   * parameters.
   */
  void erase(KeyType key, FileIdType file_id) {
    MutexGuard guard(lock_);
    erase_with_lock(key, file_id);
  }

  /**
   * \brief Erases all records with the same key.
   * \param key the key to be erased.
   */
  void erase(KeyType key) {
    MutexGuard guard(lock_);
    index_.erase(key);
  }

  Status erase_string_val(const string &key, const string &value) {
    KeyType erase_key = 0;
    FileIdType erase_value = 0;
    try {
      erase_key = lexical_cast<KeyType>(key);
    } catch(boost::bad_lexical_cast e) {
      return Status(-EINVAL, "RangeIndex::erase_string_val: Bad cast");
    }
    if (value.empty()) {
      erase(erase_key);
    } else {
      try {
        erase_value = lexical_cast<FileIdType>(value);
      } catch(boost::bad_lexical_cast e) {
        return Status(-EINVAL, "RangeIndex::erase_string_val: Bad cast");
      }
      erase(erase_key, erase_value);
    }
    return Status::OK;
  }

  /*
  Status search(const RpcRangeQuery &query, FileIdVector* results) {
    CHECK_NOTNULL(results);
    KeyType lower = std::numeric_limits<KeyType>::min();
    KeyType upper = std::numeric_limits<KeyType>::max();
    try {
      if (!query.lower.empty()) {
        lower = lexical_cast<KeyType>(query.lower);
      }
      if (!query.upper.empty()) {
        upper = lexical_cast<KeyType>(query.upper);
      }
    } catch (...) {  // NOLINT
      return Status(-1,
                    "Failed to parse upper and lower value of range query.");
    }
    this->search(lower, query.lower_open, upper, query.upper_open,
                 results);
    return Status::OK;
  }
  */

  /**
   * \brief Search files of which the keys fall into the key range [low, upper]
   * \tparam K the key type (e.g., uint64, int, float).
   * \param[in] lower the low bound of keys to search.
   * \param[in] upper the upper bound of keys to search.
   * \param[out] results results will be filled with the file IDs that
   * \param[in] open_range if it sets true, then the results do not include the
   * files which keys equals to both end-points.
   * represent the files satisified the key range query.
   */
  void search(KeyType lower, bool lower_open,
              KeyType upper, bool upper_open,
              FileIdVector *results) {
    CHECK_NOTNULL(results);
    MutexGuard guard(lock_);

    typename IndexMap::iterator iter;
    if (lower_open) {
      iter = index_.upper_bound(lower);
    } else {
      iter = index_.lower_bound(lower);
    }
    for (; iter != index_.end(); ++iter) {
      if (iter->first > upper || (upper_open && iter->first >= upper)) {
        break;
      }
      results->insert(results->end(),
                      iter->second.begin(), iter->second.end());
    }
  }

  /**
   * \brief Find all files that are indexed the same key.
   * \tparam K the key type (e.g., uint64, int, float).
   * \param[in] key the key to search.
   * \param[out] results The output files.
   */
  void search(KeyType key, FileIdVector *results) {
    CHECK_NOTNULL(results);
    MutexGuard guard(lock_);
    FileIdSet *files = find_or_null(index_, key);
    if (files) {
      results->insert(results->end(), files->begin(), files->end());
    }
  }


  /// Returns the integer representing the key type.
  int key_type() const {
    return key_type_;
  }

  /**
   * \brief Returns the approximately memory consumed in bytes.
   *
   * It calculates the approx_bytes based on the assumption that the R-B-tree
   * node used in the std::map is based on the following struct:
   * ~~~~~~~~~~~~~
   * struct RBTree {
   *   std::pair<Key, FileIdType> data;
   *   char color;
   *   RBTree *left, *right, *parent;
   * }
   * ~~~~~~~~~~~~~
   *
   * Because there is no easy way to directly get the size of key type for
   * the key_type_, it just uses uint64_t to calculate the maximum possible
   * length.
   */
  size_t approx_bytes() {
    static const size_t kElementSize =
        sizeof(uint64_t) + sizeof(FileIdType)   // key and value
        + 3 * sizeof(char*)  // left/right/parent pointers // NOLINT
        + sizeof(char);  // color. // NOLINT
    MutexGuard guard(lock_);
    return index_.size() * kElementSize;
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

  /// Returns the number of unique keys.
  size_t num_keys() {
    MutexGuard guard(lock_);
    return index_.size();
  }

  bool empty() {
    MutexGuard guard(lock_);
    return index_.empty();
  }

  void clear() {
    MutexGuard guard(lock_);
    index_.clear();
  }

 protected:
  void insert(void *key, size_t key_size, FileIdType file_id) {
    CHECK_EQ(sizeof(Key), key_size);
    Key k = *static_cast<Key*>(key);
    insert(k, file_id);
  }

  void erase(void *key, size_t key_size) {
    CHECK_EQ(sizeof(Key), key_size);
    Key k = *static_cast<Key*>(key);
    erase(k);
  }

  void erase(void *key, size_t key_size, FileIdType file_id) {
    CHECK_EQ(sizeof(Key), key_size);
    Key k = *static_cast<Key*>(key);
    erase(k, file_id);
  }

  void search(void *lower, void *upper, size_t key_size,
              FileIdVector *results) {
    CHECK_EQ(sizeof(Key), key_size);
    Key lower_key = *static_cast<Key*>(lower);
    Key upper_key = *static_cast<Key*>(upper);
    search(lower_key, false, upper_key, false, results);
  }

  void search(void *key, size_t key_size, FileIdVector *results) {
    CHECK_EQ(sizeof(Key), key_size);
    Key k = *static_cast<Key*>(key);
    search(k, results);
  }

  /**
   * \brief Splits this RangeIndex based on the value range.
   *
   * It first finds the median of the value range, and separate this index
   * into two value ranges : [start, median), [median, end]
   *
   * \param[out] other another empty range index to migrate the values from
   * [median, end] to.
   * \return Returns the median value and the end value.
   *
   * \pre The 'other' must be an empty RangeIndex.
   */
  FileIdRangeType split_index(RangeIndexInterface* other_interface) {
    CHECK_NOTNULL(other_interface);
    CHECK_EQ(other_interface->key_type(), this->key_type());
    // TODO(lxu): avoid using dynamic_cast.
    RangeIndex* other = dynamic_cast<RangeIndex*>(other_interface);  // NOLINT
    CHECK(other->empty());
    size_t total_size = this->size();
    vector<FileIdType> all_file_ids;
    MutexGuard guard(lock_);
    for (const auto& iter : index_) {
      for (const auto& file_id : iter.second) {
        all_file_ids.push_back(file_id);
      }
    }
    size_t median_pos = total_size / 2;
    std::nth_element(all_file_ids.begin(), all_file_ids.begin() + median_pos,
                     all_file_ids.end());

    FileIdType median = all_file_ids[median_pos];
    FileIdType end_pos = all_file_ids.back();
    // Inserts all file id >= median to the new RangeIndex.
    for (const auto& iter : index_) {
      unordered_set<FileIdType> moved_file_ids;
      for (const auto& file_id : iter.second) {
        if (file_id >= median) {
          moved_file_ids.insert(file_id);
        }
      }
      if (!moved_file_ids.empty()) {
        other->index_[iter.first].swap(moved_file_ids);
      }
    }
    for (const auto& iter : other->index_) {
      const auto& key = iter.first;
      for (const auto& file_id : iter.second) {
        index_[key].erase(file_id);
      }
      if (index_[key].empty()) {
        index_.erase(key);
      }
    }
    // Flag these splited elements from the original index and free them later.
    return std::make_pair(median, end_pos);
  };

 private:
  typedef unordered_set<FileIdType> FileIdSet;

  typedef map<KeyType, FileIdSet> IndexMap;

  /// Caller must hold the lock.
  void erase_with_lock(KeyType key, FileIdType file_id) {
    FileIdSet *files = find_or_null(index_, key);
    if (files) {
      files->erase(file_id);
      if (files->empty()) {
        index_.erase(key);
      }
    }
  }

  IndexMap index_;

  mutex lock_;

  int key_type_;

  DISALLOW_COPY_AND_ASSIGN(RangeIndex);
};

}  // namespace index
}  // namespace vsfs
#endif  // VSFS_INDEX_RANGE_INDEX_H_
