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
#include <sys/stat.h>
#include <set>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>  // NOLINT
#include <vector>
#include "vobla/status.h"
#include "vsfs/common/types.h"
#include "vsfs/common/leveldb_store.h"

using std::mutex;
using std::set;
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
  bool exists(const string &path) const;

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
  Status create(const string &path, int mode, uid_t uid, gid_t gid,
                ObjectId *oid);

  /**
   * \brief Removes a file.
   * \param path the full path of the deleting file.
   */
  Status remove(const string &path);

  /**
   * \brief Adds an file entry into the directory structure.
   * \param parant the parent directory path.
   * \param subfile the subfile filename, not including parent paths.
   * \return Status::OK if success.
   */
  Status add_subfile(const string &parent, const string &subfile);

  Status mkdir(const string &path, mode_t mode, uid_t uid, gid_t gid);

  Status rmdir(const string &path);

  Status readdir(const string &path, vector<string>* results);  // NOLINT

 private:
  /// The persistent storage to store the namespace.
  unique_ptr<LevelDBStore> store_;

  struct Metadata {
    string full_path;
    ObjectId object_id;
    uint32_t mode;
    uint32_t gid;
    uint32_t uid;
    uint64_t size;  // size should put into object store.
    double atime;
    double ctime;
    double mtime;
    // mutex?
  };

  /// Maintains the directory relationship with its sub files/dirs.
  struct Directory {
    set<string> subfiles;
  };

  /// Dir path to the directory structure.
  typedef unordered_map<string, Directory> DirectoryMap;

  DirectoryMap directories_;

  /// The namespace !!!
  unordered_map<string, Metadata> metadata_map_;

  /// A reversed map for fast lookup from object id to the file path.
  unordered_map<ObjectId, string> id_to_path_map_;

  ObjectId next_obj_id_;

  mutex mutex_;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_NAMESPACE_H_
