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

#include <array>
#include <initializer_list>
#include <mutex>
#include <unordered_map>
#include <utility>
#include "vsfs/common/thread.h"

using std::array;
using std::mutex;
using std::pair;
using std::unordered_map;

// TODO(eddyxu): move to common or vobla

namespace vsfs {
namespace masterd {

template <typename Key, typename T, int Size = 1024>
class MTHashMap {
 public:
  typedef unordered_map<Key, T> hashmap_type;
  typedef typename hashmap_type::key_type key_type;
  typedef typename hashmap_type::mapped_type mapped_type;
  typedef typename hashmap_type::value_type value_type;
  typedef typename hashmap_type::size_type size_type;
  typedef typename hashmap_type::hasher hasher;

  MTHashMap() = default;

  /// Copy constructor.
  MTHashMap(const MTHashMap& rhs);

  /// Move constructor.
  MTHashMap(MTHashMap&& rhs);

  /// Construct from initializer list.
  MTHashMap(std::initializer_list<value_type> values);  // NOLINT

  // It should not being override.
  ~MTHashMap() = default;

  /**
   * \brief Returns a reference to the mapped value of the element with key k
   * in the MTHashMap.
   *
   * If `key` does not match the key of any element in the container, the
   * function throws an out_of_range exception.
   */
  mapped_type& at(const key_type& key) {
    auto bucket = bucket_id(key);
    MutexGuard guard(buckets_[bucket].mutex_);
    return buckets_[bucket].data_.at(key);
  }

  const mapped_type& at(const key_type& k) const;

  mapped_type& operator[] (const key_type& k);

  mapped_type& operator[] (key_type&& k);

  bool empty() const {
    for (const auto& bucket : buckets_) {
      if (!bucket.data_.empty()) {
        return false;
      }
    }
    return true;
  }

  /// Returns the number of elements in all buckets.
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

  void lock(const key_type& key);

  void unlock(const key_type& unlock);

 private:
  /**
   * \brief A Thread-safe hash bucket.
   */
  struct MTBucket {
    // The mutex to protect one bucket.
    mutex mutex_;

    // Finer-grained hash map.
    hashmap_type data_;
  };

  typedef MTBucket bucket_type;

  int bucket_id(const key_type& key) const {
    return buckets_[0].data_.hash_function()(key) % buckets_.size();
  }

  array<bucket_type, Size> buckets_;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_MT_HASHMAP_H_
