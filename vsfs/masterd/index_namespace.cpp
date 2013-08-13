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
#include "vsfs/common/path_util.h"
#include "vsfs/common/leveldb_store.h"
#include "vsfs/common/thread.h"
#include "vsfs/masterd/index_namespace.h"
#include "vsfs/rpc/thrift_utils.h"

using vobla::contain_key;
using vobla::find_or_null;
using vsfs::LevelDBStore;

namespace vsfs {
namespace masterd {

string serialize_namespace_node(const IndexNamespaceNode &node) {
  return ThriftUtils::serialize(node);
}

IndexNamespace::IndexNamespace(const string &dbpath)
    : store_(new LevelDBStore(dbpath)) {
}

IndexNamespace::IndexNamespace(LevelDBStore* store) : store_(store) {
}

IndexNamespace::~IndexNamespace() {
}

Status IndexNamespace::init() {
  Status status = store_->open();
  if (!status.ok()) {
    return status;
  }
  MutexGuard guard(lock_);
  for (const auto &key_and_ns_node : *store_) {
    const auto& key = key_and_ns_node.first;
    unique_ptr<IndexNamespaceNode> node(new IndexNamespaceNode);
    if (!ThriftUtils::deserialize(key_and_ns_node.second, node.get())) {
      LOG(ERROR) << "Failed to deserialize namespace node: " << key;
      return Status(-1, "Failed to deserialize namespace node.");
    }
    nodes_.insert(std::make_pair(key, std::move(node)));
  }
  return Status::OK;
}

string IndexNamespace::cleanup_path(const string &path) const {
  size_t pos = path.size() - 1;
  while (pos && path[pos] == '/') {
    pos--;
  }
  return path.substr(0, pos+1);
}

Status IndexNamespace::insert(const string &path, const string &name) {
  // TODO(eddyxu): optimize the concurrent control here.
  const string key = cleanup_path(path);
  MutexGuard guard(lock_);
  if (contain_key(nodes_, key)) {
    auto node = nodes_[key].get();
    if (node->names.count(name)) {
      VLOG(1) << "Name index [" << name << "] have already existed on path: "
              << key;
      return Status(-EEXIST, "The named index have existed.");
    }
  } else {
    nodes_.insert(std::make_pair(
        key, unique_ptr<IndexNamespaceNode>(new IndexNamespaceNode)));
    nodes_[key]->path = key;
  }

  auto node = nodes_[key].get();
  node->names.insert(name);

  string value = serialize_namespace_node(*node);
  auto status = store_->put(key, value);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to insert IndexNamespace record into DB: key: "
               << key << " Reason: " << status.message();
    VLOG(0) << "Roll back the in-memory changes...";
    node->names.erase(name);
  }
  return status;
}

Status IndexNamespace::remove(const string &path, const string &name) {
  const string key = cleanup_path(path);
  MutexGuard guard(lock_);
  auto node = find_or_null(nodes_, key);
  if (!node || !(*node)->names.count(name)) {
    return Status(-ENOENT, "The index does not exist.");
  }
  (*node)->names.erase(name);

  string value = serialize_namespace_node(*(*node));
  auto status = store_->put(key, value);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to update IndexNamespace record for key: " << key
               << " Reason: " << status.message();
    VLOG(0) << "Roll back the in-memory deletion...";
    (*node)->names.insert(name);
  }
  return status;
}

Status IndexNamespace::remove(const string &path) {
  auto key = cleanup_path(path);
  MutexGuard guard(lock_);
  auto node = find_or_null(nodes_, key);
  if (!node) {
    VLOG(0) << "Attempt to remove an nonexisted path: " << key;
    return Status(-ENOENT, "The directory is not existed.");
  }
  auto status = store_->remove(key);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove IndexNamespace record for key: " << key
               << " Reason: " << status.message();
  } else {
    nodes_.erase(key);
  }
  return status;
}

bool IndexNamespace::have(const string &path, const string &name) {
  const auto key = cleanup_path(path);
  MutexGuard guard(lock_);
  auto node = find_or_null(nodes_, key);
  return node && (*node)->names.count(name);
}

Status IndexNamespace::find(const string &file_name, const string &name,
                          string *index_path) const {
  CHECK_NOTNULL(index_path);
  // TODO(eddyxu): add multi-thread support.
  // TODO(eddyxu): use unordered_map to accellerate name lookups.
  size_t pos = string::npos;
  while (pos > 0) {
    string partial_path = file_name.substr(0, pos);
    const auto node_pointer = find_or_null(nodes_, partial_path);
    if (node_pointer && (*node_pointer)->names.count(name)) {
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
  if (node_pointer && (*node_pointer)->names.count(name)) {
    *index_path = "/";
    return Status::OK;
  }

  return Status(-ENOENT, "The index does not exist.");
}

vector<string> IndexNamespace::collect(const string &root, const string &name) {
  vector<string> indices;
  MutexGuard guard(lock_);
  auto iter = nodes_.lower_bound(root);
  while (iter != nodes_.end()) {
    const string& path = iter->first;
    if (!boost::algorithm::starts_with(path, root)) {
      break;
    }
    if (iter->second->names.count(name)) {
      indices.push_back(path);
    }
    ++iter;
  }
  return indices;
}

vector<string> IndexNamespace::get_indices(const string& root, bool recursive) {
  vector<string> results;
  MutexGuard guard(lock_);
  if (recursive) {
    auto iter = nodes_.lower_bound(root);
    while (iter != nodes_.end()) {
      const string& path = iter->first;
      if (!boost::algorithm::starts_with(path, root)) {
        break;
      }
      for (const auto& name : iter->second->names) {
        results.push_back(PathUtil::index_path(path, name));
      }
    }
  } else {
    auto names = get_index_names(root);
    for (const auto& name : names) {
      results.push_back(PathUtil::index_path(root, name));
    }
  }
  return results;
}

vector<string> IndexNamespace::get_index_names(const string &path) {
  vector<string> names;
  MutexGuard guard(lock_);
  const auto node_pointer = find_or_null(nodes_, path);
  if (node_pointer) {
    names.assign((*node_pointer)->names.begin(),
                 (*node_pointer)->names.end());
  }
  return names;
}

}  // namespace masterd
}  // namespace vsfs
