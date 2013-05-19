/**
 * \file index_partition_manager.h
 *
 * \brief Manages each index as several smaller partitions.
 *
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
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
#include "vsfs/common/key_value_store.h"

using std::mutex;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using vobla::ConsistentHashMap;
using vobla::Status;

namespace vobla {
class LevelDBStore;
}

namespace vsfs {
namespace masterd {

/**
 * \class PartitionManager "vsfs/masterd/partition_manager.h"
 * \brief The manager of index partitions.
 */
class PartitionManager {
 public:
  /**
   * \note Note that here the hash value is not the full SHA1 or MD5 value,
   */
  typedef uint64_t HashValueType;

  typedef ConsistentHashMap<HashValueType, string> PartitionMap;

  explicit PartitionManager(const string &file_path);

  /**
   * \brief Constructs PartitionManager with a key value store object.
   *
   * \note This constructor should only be used for dependancy injection
   * purpose.
   */
  explicit PartitionManager(KeyValueStore* store);

  virtual ~PartitionManager() = default;

  Status insert(const string &full_index_path);

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

  /**
   * \brief Save the partition map to disk.
   */
  Status save();

  /**
   * \brief Loads the content of partition map from disk.
   */
  Status load();

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
