/*
 * Copyright 2013 (c) Ziling Huang <hzlgis@gmail.com>
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

#ifndef VSFS_MASTERD_INDEX_SERVER_MANAGER_H_
#define VSFS_MASTERD_INDEX_SERVER_MANAGER_H_

#include <mutex>
#include <string>
#include <vector>
#include "vobla/consistent_hash_map.h"
#include "vobla/macros.h"
#include "vobla/status.h"
#include "vsfs/rpc/vsfs_types.h"

using std::mutex;
using std::string;
using std::vector;
using vobla::Status;
using vsfs::rpc::NodeInfo;

namespace vsfs {
namespace masterd {

/**
 * \class IndexServerManager "vsfs/masterd/index_server_manager.h"
 * \brief It maintains all the IndexServers on a consistent hashing ring,
 * and assigns each server a hash range to feed indices.
 *
 * \note This class is thread-safe.
 */
class IndexServerManager {
 public:
  typedef uint64_t HashValueType;

  /// Default constructor.
  IndexServerManager();

  /// Deconstructor.
  virtual ~IndexServerManager();

  /**
   * \brief add a node at a random position on the ring.
   * \note Index Server call this function when join.
   */
  Status add(const NodeInfo& node);

  /**
   * \brief add a node at a client specified position on the ring.
   * \note Index Server call this function when join.
   */
  Status add(HashValueType pos, const NodeInfo& node);

  /**
   * \brief remove a node
   */
  Status remove(HashValueType pos);

  /**
   * \brief Given a hash of the partitioned index's path, get the
   * corresponding Index Server based on the position of the hash in
   * the hash ring.
   * \param [in] path_hash the hash value of the path of the index partition.
   * \param [out] node the Node's information.
   */
  Status get(HashValueType path_hash, NodeInfo* node); // NOLINT

  /**
   * \brief Get the number of physical nodes currently in the ring.
   */
  size_t num_nodes();

  vector<HashValueType> get_partitions();

  /**
   * \brief Get the next server given a path_hash.
   * \note This function can only be used when enable_server_map flag is
   * set to be true, otherwise it will return error status.
   * It will add a new record in PathHashToNodeMap.
   */
  // Status get_succ_server(const NodeInfo& this_node, NodeInfo* succ_node);

 private:
  // const unsigned int replicas_;

  // TODO(Ziling): make the key type SHA1 in the future.
  /// Mapping from partitioned file path to NodeInfo.
  vobla::ConsistentHashMap<HashValueType, NodeInfo> ring_;

  /// Global lock on ring_.
  mutex lock_;

  DISALLOW_COPY_AND_ASSIGN(IndexServerManager);
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_INDEX_SERVER_MANAGER_H_
