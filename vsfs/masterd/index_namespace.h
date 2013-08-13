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

#ifndef VSFS_MASTERD_INDEX_NAMESPACE_H_
#define VSFS_MASTERD_INDEX_NAMESPACE_H_

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include "vsfs/masterd/index_namespace_interface.h"
#include "vsfs/masterd/masterd_types.h"

using std::mutex;
using std::set;
using std::string;
using std::unique_ptr;
using std::map;
using std::vector;

namespace vsfs {
class LevelDBStore;

namespace masterd {

/**
 * \class IndexNamespace
 * \brief It manages the 'namespace' for file indices.
 *
 * It works as a write-through cache of file-index mapping. The indices
 * namespace is stored in the LevelDBStore for persistence.
 */
class IndexNamespace : public IndexNamespaceInterface {
 public:
  explicit IndexNamespace(const string &dbpath);

  /**
   * \brief Initializes an IndexNamespace with *unopened* LevelDBStore.
   * \param store an instance of LevelDBStore. The ownership of 'store' is
   * transferred to this IndexNamespace.
   */
  explicit IndexNamespace(vsfs::LevelDBStore* store);

  virtual ~IndexNamespace();

  /**
   * \brief Load the data from LevelDBStore.
   */
  virtual Status init();

  /**
   * \see IndexNamespaceInterface::insert
   * \pre This map is initialized.
   */
  virtual Status insert(const string &path, const string &name);

  virtual Status remove(const string &path, const string &name);

  virtual Status remove(const string &path);

  /// Returns true if there is an index with 'name' on path.
  virtual bool have(const string &path, const string &name);

  virtual Status find(const string &file_name, const string &name,
                      string *index_path) const;

  virtual vector<string> collect(const string &root, const string &name);

  /**
   * \brief Gets all indices under the root.
   * \param root the top directory.
   * \param recursive if it is true, then recursively to traversel the whole
   * sub namespace under 'root'
   * \returns a vector of full index paths.
   */
  virtual vector<string> get_indices(const string& root, bool recursive);

  virtual vector<string> get_index_names(const string &path);

 private:
  /// Removes all '/' at the end of path.
  string cleanup_path(const string &path) const;

  /// A direct mapping from the path to the IndexPathNode.
  map<string, unique_ptr<IndexNamespaceNode>> nodes_;

  unique_ptr<vsfs::LevelDBStore> store_;

  /// Global lock on nodes_;
  mutex lock_;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_INDEX_NAMESPACE_H_
