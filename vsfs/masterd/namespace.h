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
#include <gtest/gtest_prod.h>
#include <sys/stat.h>
#include <memory>
#include <mutex>  // NOLINT
#include <set>
#include <string>
#include <unordered_map>  // NOLINT
#include <vector>
#include "vobla/status.h"
#include "vsfs/common/types.h"
#include "vsfs/masterd/masterd_types.h"

using std::mutex;
using std::set;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using vobla::Status;

namespace vsfs {

class LevelDBStore;
class RpcFileInfo;

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

  /**
   * \brief Finds the object id by the file path.
   *
   * An object ID is the global unique ID in the system, which is similar to
   * the inode number in local file system or FID in Lustre.
   */
  Status object_id(const string &path, ObjectId *oid);

  /// Finds the file path by the given file id.
  Status file_path(ObjectId oid, string *path);

  /// Finds files in batch by the given file IDs.
  Status find_files(const vector<ObjectId>& object_ids,
                    vector<string>* paths);

  void find_objects(const vector<string>& paths,
                    vector<ObjectId>* object_ids);

  /**
   * \brief Access the attributes of a file or directory.
   * \param[in] path the full path of the targeted file.
   * \param[out] info filled with the metadata of file.
   */
  Status getattr(const string& path, RpcFileInfo *info);

  /**
   * \brief Sets the attribute of one file.
   */
  Status setattr(const string& path, const RpcFileInfo& info);

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
   * \param[in] path the full path of the deleting file.
   * \param[out] oid the old Object ID of the deleting file.
   */
  Status remove(const string &path, ObjectId* oid);

  /**
   * \brief Adds a file entry into the directory structure.
   * \param parant the parent directory path.
   * \param subfile the subfile filename, not including parent paths.
   * \return Status::OK if success.
   */
  Status add_subfile(const string &parent, const string &subfile);

  /**
   * \brief Removes a file entry from the given directory structure.
   */
  Status remove_subfile(const string &parent, const string &subfile);

  /// Creates a new directory.
  Status mkdir(const string &path, mode_t mode, uid_t uid, gid_t gid);

  /// Removes a directory.
  Status rmdir(const string &path);

  /// Read all sub files.
  Status readdir(const string &path, vector<string>* results);  // NOLINT

 private:
  FRIEND_TEST(NamespaceTest, TestObjectIdDistribution);

  /**
   * \brief Calculate object id from the file path.
   * \note the caller needs to hold the mutex_.
   */
  ObjectId get_object_id(const string &path);

  /// Stores file/dir metadata into LevelDBStore.
  Status store_metadata(const string &path, const FileMetadata& metadata);

  /// The persistent storage to store the namespace.
  unique_ptr<LevelDBStore> store_;

  /// Maintains the directory relationship with its sub files/dirs.
  struct Directory {
    set<string> subfiles;
  };

  /// Dir path to the directory structure.
  typedef unordered_map<string, Directory> DirectoryMap;

  DirectoryMap directories_;

  /// The namespace !!!
  unordered_map<string, FileMetadata> metadata_map_;

  /// A reversed map for fast lookup from object id to the file path.
  unordered_map<ObjectId, string> id_to_path_map_;

  ObjectId next_obj_id_;

  mutex mutex_;
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_NAMESPACE_H_
