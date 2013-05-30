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

#include <boost/algorithm/string/predicate.hpp>
#include <glog/logging.h>
#include <utility>
#include <string>
#include <vector>
#include "vobla/map_util.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/masterd/index_path_map.h"

using vobla::contain_key;
using vobla::find_or_null;
using vsfs::LevelDBStore;

namespace vsfs {
namespace masterd {

IndexPathMap::IndexPathMap(const string &dbpath)
    : store_(new LevelDBStore(dbpath)) {
}

IndexPathMap::IndexPathMap(LevelDBStore* store) : store_(store) {
}

IndexPathMap::~IndexPathMap() {
}

Status IndexPathMap::init() {
  Status status = store_->open();
  if (!status.ok()) {
    return status;
  }
  MutexGuard guard(lock_);
  return Status::OK;
}

string IndexPathMap::cleanup_path(const string &path) const {
  size_t pos = path.size() - 1;
  while (pos && path[pos] == '/') {
    pos--;
  }
  return path.substr(0, pos+1);
}

Status IndexPathMap::insert(const string &path, const string &name) {
  // TODO(eddyxu): optimize the concurrent control here.
  // TODO(eddyxu): maybe it is not IndexPathMap's responsibility to check path?
  const string key = cleanup_path(path);
  MutexGuard guard(lock_);
  if (!contain_key(nodes_, key)) {
    nodes_.insert(std::make_pair(
            key, unique_ptr<IndexPathNode>(new IndexPathNode)));
  }
  IndexPathNode* node = nodes_[key].get();
  if (node->index_names.find(name) != node->index_names.end()) {
    VLOG(1) << "Name index [" << name << "] has already existed on path: "
            << key;
    return Status(-EEXIST, "The named index has existed.");
  }
  node->index_names.insert(name);
  return Status::OK;
}

Status IndexPathMap::remove(const string &path, const string &name) {
  const string key = cleanup_path(path);
  MutexGuard guard(lock_);
  auto node = find_or_null(nodes_, key);
  if (!node) {
    return Status(-ENOENT, "The index does not exist.");
  }
  (*node)->index_names.erase(name);
  return Status::OK;
}

Status IndexPathMap::find(const string &file_name, const string &name,
                                string *index_path) const {
  CHECK_NOTNULL(index_path);
  // TODO(eddyxu): add multi-thread support.
  // TODO(eddyxu): use unordered_map to accellerate name lookups.
  size_t pos = string::npos;
  while (pos > 0) {
    string partial_path = file_name.substr(0, pos);
    const auto node_pointer = find_or_null(nodes_, partial_path);
    if (node_pointer && (*node_pointer)->has_name(name)) {
        *index_path = partial_path;
        return Status::OK;
    }
    pos = partial_path.find_last_of('/');
    if (pos == string::npos) {
      break;
    }
  }
  // Check root directory.
  const auto node_pointer = find_or_null(nodes_, "/");
  if (node_pointer && (*node_pointer)->has_name(name)) {
    *index_path = "/";
    return Status::OK;
  }

  return Status(-ENOENT, "The index does not exist.");
}

vector<string> IndexPathMap::collect(const string &root, const string &name) {
  vector<string> indices;
  MutexGuard guard(lock_);
  auto iter = nodes_.lower_bound(root);
  while (iter != nodes_.end()) {
    const string& path = iter->first;
    if (!boost::algorithm::starts_with(path, root)) {
      break;
    }
    if (iter->second->has_name(name)) {
      indices.push_back(path);
    }
    ++iter;
  }
  return indices;
}

vector<string> IndexPathMap::get_index_names(const string &path) {
  vector<string> names;
  MutexGuard guard(lock_);
  const auto node_pointer = find_or_null(nodes_, path);
  if (node_pointer) {
    names.assign((*node_pointer)->index_names.begin(),
                 (*node_pointer)->index_names.end());
  }
  return names;
}

}  // namespace masterd
}  // namespace vsfs
