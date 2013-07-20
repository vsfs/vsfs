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

#ifndef VSFS_MASTERD_NAMESPACE_H_
#define VSFS_MASTERD_NAMESPACE_H_

#include <boost/utility.hpp>
#include <map>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>  // NOLINT
#include <vector>
#include "vobla/status.h"
#include "vsfs/common/types.h"
#include "vsfs/common/leveldb_store.h"

using std::mutex;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using vobla::Status;

namespace vsfs {
namespace masterd {

/**
 * \class Namespace "vsfs/masterd/namespace.h"
 * \brief The unified namespace for both file namespace and index namespace.
 */
class Namespace : boost::noncopyable {
 public:
  explicit Namespace(const string &metadb);

  /**
   * \brief Constructs a Namespace object with mocked store.
   *
   * It should only be used for the testing purpose.
   */
  explicit Namespace(LevelDBStore *mock_store);

  virtual ~Namespace();

  Status init();

  /// Returns true if the path is existed.
  bool exists(const string &path);

  /// Finds the file id by the file path.
  Status file_id(const string &path, ObjectId *oid);

  /// Finds the file path by the given file id.
  Status file_path(ObjectId oid, string *path);

  /// Finds files in batch by the given file IDs.
  Status find_files(const vector<ObjectId>& file_ids,
                    vector<string>* paths);

  /**
   * \brief Creates a file and returns its file id.
   * \param[in] path the file path
   * \param[out] oid the new created file ID for this file.
   * \return Status::OK if success.
   */
  Status create(const string &path, ObjectId *oid);

  /**
   * \brief Remove a file.
   */
  Status remove(const string &path);

  Status create_dir(const string &path);

  Status listdir(const string &path, vector<string>* paths);

 private:
  /// The persistent storage to store the namespace.
  unique_ptr<LevelDBStore> store_;

  unordered_map<ObjectId, string> id_to_path_map_;

  mutex mutex_;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_NAMESPACE_H_
