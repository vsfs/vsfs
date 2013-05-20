/*
 * Copyright 2011-2013 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_MASTERD_PARTITION_MANAGER_H_
#define VSFS_MASTERD_PARTITION_MANAGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "vobla/consistent_hash_map.h"
#include "vobla/macros.h"
#include "vobla/status.h"

using std::mutex;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using vobla::ConsistentHashMap;
using vobla::Status;

namespace vsfs {
class KeyValueStore;

/**
 * \namespace masterd
 * \brief Master Node of VSFS
 */
namespace masterd {

/**
 * \class PartitionManager "vsfs/masterd/partition_manager.h"
 * \brief The manager of index partitions.
 *
 * It works as a write-through cache of partition location information in the
 * RAM, the content in RAM is consistent with the persistent K-V store (a.k.a
 * storing in LevelDB).
 */
class PartitionManager {
 public:
  /**
   * \note Note that here the hash value is not the full SHA1 or MD5 value,
   */
  typedef uint64_t HashValueType;

  typedef ConsistentHashMap<HashValueType, string> PartitionMap;

  /**
   * \brief Constructs a PartitionManager with the file path to the local
   * store.
   */
  explicit PartitionManager(const string &file_path);

  /**
   * \brief Constructs PartitionManager with a key value store object.
   *
   * \note This constructor should only be used for dependancy injection
   * purpose.
   */
  explicit PartitionManager(KeyValueStore* store);

  virtual ~PartitionManager() = default;

  /**
   * \brief Loads the content of partition map from DB.
   */
  Status load();

  /**
   * \brief Save the partition map to disk.
   */
  Status save();

  /**
   * \brief Add a new Index and initializes an empty partition for it.
   * \param full_index_path
   */
  Status add_index(const string &full_index_path);

  void remove(const string &full_index_path);

  Status insert(const string &full_index_path,
                HashValueType sep);

  Status remove(const string &full_index_path,
                HashValueType sep);

  string get_partition_path(const string &full_index_path,
                            HashValueType hash);

  Status get_all_partition_paths(const string &full_index_path,
                                 vector<string> *paths);

  /**
   * \brief Copy an instance of PartitionMap.
   * \param[in] full_index_path the actual path for this index, as same as used
   * in the above functions.
   * \param[out] partition_map it will be filled with the content of the
   * consistent hash ring for the index on 'full_index_path'.
   *
   * \note The instance of PartitionMap copied out is not guaranteed to be
   * consistent with the content in PartitionManager. Future work is need
   * to handle the concurrent modifications on PartitionMap.
   */
  Status copy_partition_map(const string &full_index_path,
                            PartitionMap *partition_map);

 private:
  string partition_map_to_string(const PartitionMap& pm);

  struct Partition {
    mutex mutex_;
    size_t refcount_;
    PartitionMap partitions_;
  };

  typedef unordered_map<string, unique_ptr<Partition>>
      PartitionMapByIndexPath;

  string file_path_;

  PartitionMapByIndexPath partition_map_by_index_path_;

  mutex mutex_;

  unique_ptr<KeyValueStore> store_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(PartitionManager);
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_PARTITION_MANAGER_H_
