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

#ifndef VSFS_MASTERD_PARTITION_MANAGER_INTERFACE_H_
#define VSFS_MASTERD_PARTITION_MANAGER_INTERFACE_H_

#include <string>
#include <vector>
#include "vobla/status.h"
#include "vobla/consistent_hash_map.h"

using std::string;
using std::vector;
using vobla::ConsistentHashMap;
using vobla::Status;

namespace vsfs {
namespace masterd {

class PartitionManagerInterface {
 public:
  typedef int64_t HashValueType;

  typedef ConsistentHashMap<HashValueType, string> PartitionMap;

  virtual ~PartitionManagerInterface() {}

  /// Initialize PartitionManager.
  virtual Status init() = 0;

  /// Add a new Index and initializes an empty partition for it.
  virtual Status add_index(const string& key) = 0;

  /// Remove one index and all its partitions.
  virtual Status remove_index(const string& key) = 0;

  /// Adds an index partition at the hash value `sep`.
  virtual Status add_partition(const string& key, HashValueType sep) = 0;

  /// Removes an index partition at the hash value 'sep'.
  virtual Status remove_partition(const string& key, HashValueType sep) = 0;

  /// Gets the path of a partition.
  virtual string get_partition_path(const string &full_index_path,
                                    HashValueType hash) = 0;


  /// Gets all partition's paths
  virtual Status get_all_partitions(const string &full_index_path,
                                    vector<string> *paths) = 0;

  /**
   * \brief Copys an instance of PartitionMap.
   * \param[in] full_index_path the actual path for this index, as same as used
   * in the above functions.
   * \param[out] partition_map it will be filled with the content of the
   * consistent hash ring for the index on 'full_index_path'.
   */
  virtual Status copy_partition_map(const string &full_index_path,
                                    PartitionMap *partition_map) = 0;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_PARTITION_MANAGER_INTERFACE_H_
