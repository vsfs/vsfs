/*
 * Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_MASTERD_MT_HASHMAP_H_
#define VSFS_MASTERD_MT_HASHMAP_H_

#include <boost/iterator/iterator_facade.hpp>
#include <glog/logging.h>
#include <algorithm>
#include <array>
#include <initializer_list>
#include <mutex>
#include <unordered_map>
#include <utility>
#include "vsfs/common/thread.h"

using std::unordered_map;

namespace vsfs {
namespace masterd {

/**
 * \brief A fine-grained access-control thread-safe hash map.
 *
 * It first divides the hash map into buckets, based on the hash(key) range.
 * Each bucket has its own mutex to protect the data within this bucket.
 *
 * \note It only guarentees thread-safe for insert/find/erase/at/[] operations,
 * but not for operations that involve multiple buckets. These cross-buckets
 * operations are not the optimization target for now.
 *
 * \todo (eddyxu): move to vsfs/common or vobla.
 */
template <typename Key, typename T, int Size = 1024>
class MTHashMap {
 public:
  /// The underlying hash map container type.
  typedef unordered_map<Key, T> hashmap_type;
  /// The type of key
  typedef typename hashmap_type::key_type key_type;
  /// The type of value
  typedef typename hashmap_type::mapped_type mapped_type;
  /// The type of pair<Key, Value>
  typedef typename hashmap_type::value_type value_type;
  typedef typename hashmap_type::size_type size_type;
  /// The hash function used by the underlying hash map container.
  typedef typename hashmap_type::hasher hasher;

 private:
  class MTHashMapIterator : public boost::iterator_facade<
    MTHashMapIterator, value_type, boost::forward_traversal_tag> {
    typedef typename hashmap_type::iterator hash_iterator_type;

   public:
    /**
     * \brief Constructs a MTHashMapIterator.
     * \param mthm a pointer to its MTHashMap. It does not own this pointer.
     * \param bucket_idx the index of the bucket where this iterator is on.
     * \param it the iterator of the MTHashMap::hashmap_type.
     *
     * The end() iterator has the following condition:
     * \code{.cpp}
     *   bucket_idx_ == map_->bucket_size() - 1 &&
     *   iter_ == map_->bucket_.back().end()
     * \endcode
     */
    MTHashMapIterator(MTHashMap* mthm, int bucket_idx, hash_iterator_type it)
        : map_(CHECK_NOTNULL(mthm)), bucket_idx_(bucket_idx), iter_(it) {
    }

    // ++it
    void increment() {
      if (bucket_idx_ >= map_->buckets_.size()) {
        return;
      }
      iter_++;
      if (iter_ == map_->buckets_[bucket_idx_].data_.end()) {
        bucket_idx_++;
        while (bucket_idx_ < map_->buckets_.size()) {
          if (!map_->buckets_[bucket_idx_].data_.empty()) {
            iter_ = map_->buckets_[bucket_idx_].data_.begin();
            return;
          }
          bucket_idx_++;
        }
        iter_ = map_->buckets_.back().data_.end();
      }
    }

    value_type& dereference() const {
      return *iter_;
    }

    /// it != end()?
    bool equal(MTHashMapIterator const& rhs) const {
      return bucket_idx_ == rhs.bucket_idx_
          && iter_ == rhs.iter_;
    }

   private:
    /// The pointer to its MTHashMap object.
    MTHashMap* map_;

    /// The index of bucket this iterator is on.
    int bucket_idx_;

    typename hashmap_type::iterator iter_;
  };


 public:
  typedef MTHashMapIterator iterator;

  MTHashMap() = default;

  /// Copy constructor.
  MTHashMap(const MTHashMap& rhs) {
    *this = rhs;
  }

  /// Move constructor.
  MTHashMap(MTHashMap&& rhs) {
    *this = std::move(rhs);
  }

  /// Construct from initializer list.
  MTHashMap(std::initializer_list<value_type> values);  // NOLINT

  // It should not being override.
  ~MTHashMap() = default;

  /**
   * \brief Copy assignment.
   * \note This function is not thread-safe.
   */
  MTHashMap& operator=(const MTHashMap& rhs) {
    for (int i = 0; i < buckets_.size(); i++) {
      // Only copy data fields, leave mutex alone.
      buckets_[i].data_ = rhs.buckets_[i].data_;
    }
    return *this;
  }

  /**
   * \brief Move assignment.
   * \note This function is not thread-safe.
   */
  MTHashMap& operator=(MTHashMap&& rhs) {
    for (int i = 0; i < buckets_.size(); i++) {
      // Only copy data fields, leave mutex alone.
      buckets_[i].data_ = std::move(rhs.buckets_[i].data_);
    }
    return *this;
  }

  // MTHashMap& operator=(initializer_list<value_type> values);

  bool operator==(const MTHashMap& rhs) const {
    for (int i = 0; i < buckets_.size(); i++) {
      if (buckets_[i].data_ != rhs.buckets_[i].data_) {
        return false;
      }
    }
    return true;
  }

  /**
   * \brief Returns a reference to the mapped value of the element with key k
   * in the MTHashMap.
   *
   * If `key` does not match the key of any element in the container, the
   * function throws an out_of_range exception.
   */
  mapped_type& at(const key_type& key) {
    auto idx = bucket_idx(key);
    MutexGuard guard(buckets_[idx].mutex_);
    return buckets_[idx].data_.at(key);
  }

  /**
   * \brief Returns a const reference to the mapped value of the element with
   * key k in the MTHashMap.
   */
  const mapped_type& at(const key_type& key) const {
    auto idx = bucket_idx(key);
    MutexGuard guard(buckets_[idx].mutex_);
    return buckets_[idx].data_.at(key);
  }


  /**
   * \brief Access element.
   *
   * If 'key' matches the key of an element in the container, the function
   * returns a reference to its mapped value.
   *
   * If key does not match the key of any element in the container, the
   * function inserts a new element with that key and returns a reference to
   * its mapped value.
   */
  mapped_type& operator[](const key_type& key) {
    auto idx = bucket_idx(key);
    MutexGuard guard(buckets_[idx].mutex_);
    return buckets_[idx].data_[key];
  }

  std::pair<iterator, bool> insert(const value_type& val) {
    auto idx = bucket_idx(val.first);
    MutexGuard guard(buckets_[idx].mutex_);
    auto res = buckets_[idx].data_.insert(val);
    return std::make_pair(MTHashMapIterator(this, idx, res.first), res.second);
  }

  /// Erase an element by its key.
  size_type erase(const key_type& key) {
    auto idx = bucket_idx(key);
    MutexGuard guard(buckets_[idx].mutex_);
    return buckets_[idx].data_.erase(key);
  }

  iterator find(const key_type& key) {
    auto idx = bucket_idx(key);
    MutexGuard guard(buckets_[idx].mutex_);
    auto iter = buckets_[idx].data_.find(key);
    if (iter == buckets_[idx].data_.end()) {
      return end();
    }
    return MTHashMapIterator(this, idx, iter);
  }

  /**
   * Not thread-safe and has O(bucket_size) complexity.
   */
  void clear() noexcept {
    for (auto& bucket : buckets_) {
      bucket.data_.clear();
    }
  }

  /**
   * \brief Returns true of this hash map is emtpy, that is, all buckets are
   * emtpy.
   *
   * It has O(bucket_size) complexity.
   */
  bool empty() const {
    for (const auto& bucket : buckets_) {
      if (!bucket.data_.empty()) {
        return false;
      }
    }
    return true;
  }

  /**
   * \brief Returns the number of elements in all buckets.
   *
   * It has O(bucket_size) complexity.
   */
  size_type size() const noexcept {
    size_type total_size = 0;
    for (const auto& bucket : buckets_) {
      total_size += bucket.data_.size();
    }
    return total_size;
  }

  size_type bucket_size() const noexcept {
    return buckets_.size();
  }

  /// Not thread-safe
  iterator begin() noexcept {
    for (int i = 0; i < buckets_.size(); ++i) {
      if (!buckets_[i].data_.empty()) {
        return MTHashMapIterator(this, i, buckets_[i].data_.begin());
      }
    }
    return end();
  }

  /// Not thread-safe
  /*
  const_iterator begin() const noexcept {
    for (int i = 0; i < buckets_.size(); ++i) {
      if (!buckets_[i].data_.empty()) {
        return MTHashMapIterator(this, i, buckets_[i].data_.begin());
      }
    }
    return end();
  }
  */

  /// Not thread-safe
  iterator end() noexcept {
    return MTHashMapIterator(this, buckets_.size(),
                             buckets_.back().data_.end());
  }

  /// Not thread-safe
  /*
  const_iterator end() const noexcept {
    return MTHashMapIterator(this, buckets_.size(),
                             buckets_.back().data_.end());
  }
  */

  void swap(MTHashMap& rhs) {
    std::swap(buckets_, rhs.buckets_);
  }

  /// Access the mutex for a given bucket.
  std::mutex& mutex(const key_type& key) {
    auto idx = bucket_idx(key);
    return buckets_[idx].mutex_;
  }

 private:
  /**
   * \brief A Thread-safe hash bucket.
   */
  struct MTBucket {
    // The mutex to protect one bucket.
    std::mutex mutex_;

    // Finer-grained hash map.
    hashmap_type data_;
  };

  typedef MTBucket bucket_type;

  int bucket_idx(const key_type& key) const {
    return buckets_[0].data_.hash_function()(key) % buckets_.size();
  }

  std::array<bucket_type, Size> buckets_;
};

template <typename Key, typename T, int Size>
void swap(MTHashMap<Key, T, Size>& lhs, MTHashMap<Key, T, Size>& rhs) {
  lhs.swap(rhs);
}

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_MT_HASHMAP_H_
