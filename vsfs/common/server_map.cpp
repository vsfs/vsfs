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

#include <glog/logging.h>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "vobla/range.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/server_map.h"

using std::map;
using std::string;
using std::vector;
using vobla::append_keys_from_map;
using vobla::Range;

namespace vsfs {

ServerMap::ServerMap() {
}

ServerMap::~ServerMap() {
}

Status ServerMap::add(const NodeInfo& node) {
  MutexGuard guard(lock_);
  HashValueType pos = 0;
  if (ring_.empty()) {
    // insert at a random position in range HashValueType when there is no
    // server.
    pos = static_cast<HashValueType>(rand()) *  // NOLINT
          static_cast<HashValueType>(rand());  // NOLINT
  } else {
    // if there is server in the ring, find a max range and insert at
    // the median of the range.
    Range<HashValueType> range;
    Status status = ring_.get_max_range(&range);
    if (!status.ok()) {
      return status;
    }
    if (range.lower() < range.upper()) {
      pos = (range.lower() + range.upper()) / 2;
    } else if (range.lower() == range.upper()) {
      pos = range.lower() + numeric_limits<HashValueType>::max() / 2;
    } else {
      // range crossing zero.
      pos = numeric_limits<HashValueType>::max() / 2  - range.lower() / 2
            + range.upper() / 2 + range.lower();
    }
  }
  return ring_.insert(pos, node);
}

Status ServerMap::add(HashValueType pos, const NodeInfo& node) {
  MutexGuard guard(lock_);
  if (ring_.has_key(pos)) {
    return Status(-EEXIST, "This position is already used.");
  }
  return ring_.insert(pos, node);
}

Status ServerMap::remove(HashValueType pos) {
  MutexGuard guard(lock_);
  // TODO(ziling): remove the record in redirection_map.
  if (!ring_.has_key(pos)) {
    return Status(-ENOENT, "This position does not exsit.");
  }
  return ring_.remove(pos);
}

Status ServerMap::get(HashValueType path_hash, NodeInfo* node) {
  CHECK_NOTNULL(node);
  // TODO(Ziling): use rw-lock to improve performance.
  MutexGuard guard(lock_);
  Status status = ring_.get(path_hash, node);
  if (!status.ok()) {
    return Status(-ENOENT, "Can't find any Server.");
  }
  return Status::OK;
}

vector<NodeInfo> ServerMap::get_replica_servers(
    const NodeInfo& node, size_t num_replicas) {
  vector<NodeInfo> retval;
  size_t actual_num_replicas = std::min(num_replicas, ring_.num_nodes() - 1);
  NodeInfo cur_node, succ_node;
  cur_node = node;
  MutexGuard guard(lock_);
  for (size_t i = 0; i < actual_num_replicas; i++) {
    ring_.succ_by_value(cur_node, &succ_node);
    retval.emplace_back(succ_node);
    cur_node = succ_node;
  }
  return retval;
}

size_t ServerMap::num_nodes() {
  MutexGuard guard(lock_);
  return ring_.num_nodes();
}

vector<HashValueType> ServerMap::get_partitions() {
  vector<HashValueType> partitions;
  MutexGuard lock(lock_);
  if (!ring_.empty()) {
    partitions = ring_.get_partitions();
  }
  return partitions;
}

map<HashValueType, NodeInfo> ServerMap::get_ch_ring_as_map() {
  map<HashValueType, NodeInfo> result;
  MutexGuard guard(lock_);
  auto it = ring_.begin();
  auto num_nodes = ring_.num_nodes();
  for (size_t i = 0; i < num_nodes; ++i, ++it) {
    result[it->first] = it->second;
  }
  return result;
}

}  // namespace vsfs
