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
#include <mutex>
#include <vector>
#include "vobla/status.h"
#include "vobla/map_util.h"
#include "vsfs/common/types.h"
#include "vsfs/rpc/vsfs_types.h"

using boost::call_traits;
using boost::lexical_cast;
using vobla::find_or_null;
using vobla::Status;

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

  void insert(typename call_traits<KeyType>::param_type key,
              ObjectId obj_id) {
    MutexGuard guard(lock_);
    index_[key].insert(obj_id);
  }

  /**
   * \brief Applies the updates to the index.
   */
  Status apply(const vector<RpcIndexRecordUpdateOp>& updates) {
    MutexGuard guard(lock_);
    for (const auto& update : updates) {
      KeyType key = 0;
      ObjectId file_id = 0;
      try {
        key = lexical_cast<KeyType>(update.key);
        file_id = lexical_cast<ObjectId>(update.value);
      } catch(boost::bad_lexical_cast e) {
        return Status(-EINVAL, "IndexImpl::apply: bad cast.");
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

  void erase(typename call_traits<KeyType>::param_type key, ObjectId obj_id) {
    MutexGuard guard(lock_);
    erase_with_lock(key, obj_id);
  }

  void erase(typename call_traits<KeyType>::param_type key) {
    MutexGuard guard(lock_);
    index_.erase(key);
  }

  bool empty() {
    MutexGuard guard(lock_);
    return index_.empty();
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
