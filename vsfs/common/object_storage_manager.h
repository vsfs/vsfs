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

#ifndef VSFS_COMMON_OBJECT_STORAGE_MANAGER_H_
#define VSFS_COMMON_OBJECT_STORAGE_MANAGER_H_

#include <string>
#include "vobla/status.h"
#include "vsfs/common/storage_manager.h"
#include "vsfs/common/types.h"

using std::string;
using vobla::Status;

namespace vsfs {

class FileObject;

/**
 * \class ObjectStorageManager
 * \brief The StorageManager that runs on a shared network file system or local
 * file system and access files by ObjectId.
 *
 * The way to store the file/objects in ObjectStorageManager is that it manages
 * the 'base_dir' as a two-level hash buckets:
 *  - the first level is calculated by 'ObjectID % num_subdirs'
 *  - the second level is using the object ID as filename.
 *
 *  E.g., suppose we have 'num_subdirs = 16', then the file with ObjectID 19
 *  is stored on '/base/3/19', where '/base' is the base path of this
 *  ObjectStorageManager.
 *
 *  \note ObjectStorageManager itself is not a distributed storage storage: it
 *  runs on top of existing networked file system like NFS or Lustre.
 */
class ObjectStorageManager : public StorageManager {
 public:
  enum {
    NUM_SUBFILES = 8192
  };

  ObjectStorageManager() = delete;

  /**
   * \brief Constructs a ObjectStorageManager.
   * \param base_path the base directory path.
   * \param num_subdirs the number of sub-directories on the first level.
   */
  ObjectStorageManager(const string& base_path, int num_subdirs = NUM_SUBFILES);

  virtual ~ObjectStorageManager();

  /**
   * \brief Initialize a ObjectStorageManager.
   *
   * It creates all sub-directories if they are not existed.
   */
  Status init();

  // Do nothing.
  Status destroy();

  Status open(const string& path, ObjectId obj_id, int flags, FileObject** obj);

  Status open(const string& path, ObjectId obj_id, int flags, mode_t mode,
              FileObject** obj);

  Status unlink(const string& path, ObjectId obj_id);

  /**
   * \brief ObjectStorageManager does not support "directory" semantic. So it
   * always returns OK.
   */
  Status mkdir(const string& path, mode_t mode);

  /// Always returns OK.
  Status rmdir(const string& path);

 private:
  string translate_path(ObjectId obj_id) const;

  /// The base directory to store the objects.
  string base_path_;

  int num_subdirs_;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_OBJECT_STORAGE_MANAGER_H_
