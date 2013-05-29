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

#ifndef VSFS_MASTERD_INDEX_PATH_MAP_H_
#define VSFS_MASTERD_INDEX_PATH_MAP_H_

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include "vsfs/masterd/index_path_map_interface.h"

using std::mutex;
using std::set;
using std::string;
using std::unique_ptr;
using std::map;
using std::vector;

namespace vsfs {
namespace masterd {

class IndexPathMap : public IndexPathMapInterface {
 public:
  IndexPathMap() = default;

  virtual ~IndexPathMap();

  virtual Status insert(const string &path, const string &name);

  virtual Status remove(const string &path, const string &name);

  virtual Status find(const string &file_name, const string &name,
                      string *index_path) const;

  virtual Status collect(const string &root, const string &name,
                         vector<string>* indices) const;

  virtual Status get_index_names(const string &path,
                                 vector<string>* names) const;

 private:
  /**
   * \brief The internal structure to represent a directory with the names of
   * the indices on this directory.
   */
  struct IndexPathNode {
    set<string> index_names;

    mutex lock;

    bool has_name(const string &name) const {
      return index_names.find(name) != index_names.end();
    }
  };

  /// Removes all '/' at the end of path.
  string cleanup_path(const string &path) const;

  /// A direct mapping from the path to the IndexPathNode.
  map<string, unique_ptr<IndexPathNode>> nodes_;

  /// Global lock on nodes_;
  mutex lock_;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_INDEX_PATH_MAP_H_
