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

#ifndef VSFS_MASTERD_INDEX_NAMESPACE_INTERFACE_H_
#define VSFS_MASTERD_INDEX_NAMESPACE_INTERFACE_H_

#include <string>
#include <vector>
#include "vobla/status.h"

using std::string;
using std::vector;
using vobla::Status;

namespace vsfs {
namespace masterd {

/**
 * \class IndexNamespaceInterface 'vsfs/masterd/index_path_map.h'
 * \brief The interfaces for IndexNamespace.
 * IndexNamespace is a mapping to store the path for index.
 */
class IndexNamespaceInterface {
 public:
  virtual ~IndexNamespaceInterface() {}

  /**
   * \brief Initialize this IndexNamespace.
   */
  virtual Status init() = 0;

  /**
   * \brief Inserts an index on 'path' with 'name'.
   * \param path the root path of the index starts.
   * \param name the name of the index.
   */
  virtual Status insert(const string &path, const string &name) = 0;

  /// Removes an index with 'name' on 'path'
  virtual Status remove(const string &path, const string &name) = 0;

  /// Removes the all the indices on the path.
  virtual Status remove(const string &path) = 0;

  /**
   * \brief Gets the index for a path.
   *
   * It returns the lowest path that has the index name 'name', in the
   * 'file_name', e.g., assuming there are two indices with name 'energy' on
   * '/home/foo' and '/home/foo/data0', then index_path for
   * (file_name='/home/foo/data0/bar/more.txt', name='energy') is
   * '/home/foo/data0'.
   *
   * \param[in] file_name the path of the file
   * \param[in] name the name of the desired index.
   * \param[out] index_path the path of the index it found.
   */
  virtual Status find(const string &file_name, const string &name,
                      string *index_path) const = 0;

  /**
   * \brief Collects all the indices under 'root' that have the 'name'.
   * \param[in] root the root directory to look for named indices.
   * \param[in] name the name of indices to be searched.
   * \return indices the paths of the returned indices.
   */
  virtual vector<string> collect(const string &root, const string &name) = 0;

  virtual vector<string> get_indices(const string& root, bool recursive) = 0;

  /**
   * \brief Gets all the index names on this path.
   * \param[in] path Find indices on this path.
   * \return a vector of string filled with the index names on this path.
   */
  virtual vector<string> get_index_names(const string &path) = 0;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_INDEX_NAMESPACE_INTERFACE_H_
