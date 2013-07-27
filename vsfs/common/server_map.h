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

#ifndef VSFS_COMMON_SERVER_MAP_H_
#define VSFS_COMMON_SERVER_MAP_H_

#include <boost/utility.hpp>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "vobla/consistent_hash_map.h"
#include "vobla/status.h"
#include "vsfs/common/types.h"
#include "vsfs/rpc/vsfs_types.h"

using std::map;
using std::mutex;
using std::string;
using std::vector;
using vobla::Status;
using vsfs::NodeInfo;

namespace vsfs {

/**
 * \class ServerMap "vsfs/common/server_map.h"
 * \brief It manages all the servers on a consistent hashing ring,
 * \note This class is thread-safe.
 */
class ServerMap : boost::noncopyable {
 public:
  /// Default constructor.
  ServerMap();

  virtual ~ServerMap();

  /**
   * \brief add a node at a random position on the ring.
   * \note Server call this function when join.
   */
  Status add(const NodeInfo& node);

  /**
   * \brief add a node at a client specified position on the ring.
   * \note Server call this function when join.
   */
  Status add(HashValueType pos, const NodeInfo& node);

  /**
   * \brief remove a node
   */
  Status remove(HashValueType pos);

  /**
   * \brief Given a hash of the path, get the corresponding Server based on the
   * position of the hash in the hash ring.
   * \param [in] path_hash the hash value of the path of the target.
   * \param [out] node the Node's information.
   */
  Status get(HashValueType path_hash, NodeInfo* node); // NOLINT

  /**
   * \brief Gets the replica servers for node.
   * \param node the node info of the requiring node.
   * \param num_replicas the number of replica servers to be returned.
   */
  vector<NodeInfo> get_replica_servers(const NodeInfo& node,
                                       size_t num_replicas);

  /**
   * \brief Get the number of physical nodes currently in the ring.
   */
  size_t num_nodes();

  /**
   * \brief Get all the partitions in the ring.
   */
  vector<HashValueType> get_partitions();

  /**
   * \brief Get the next server given a path_hash.
   * \note This function can only be used when enable_server_map flag is
   * set to be true, otherwise it will return error status.
   * It will add a new record in PathHashToNodeMap.
   */
  // Status get_succ_server(const NodeInfo& this_node, NodeInfo* succ_node);

  /**
   * \brief Obtain a full copy of Consistent Hashing Ring and stores it in a
   * std::map.
   */
  map<HashValueType, NodeInfo> get_ch_ring_as_map();

 private:
  // TODO(Ziling): make the key type SHA1 in the future.
  /// Mapping from partitioned file path to NodeInfo.
  vobla::ConsistentHashMap<HashValueType, NodeInfo> ring_;

  /// Global lock on ring_.
  mutex lock_;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_SERVER_MAP_H_
