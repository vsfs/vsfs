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

class ObjectStorageManager : public StorageManager {
 public:
  enum {
    NUM_SUBFILES = 8192
  };

  ObjectStorageManager() = delete;

  ObjectStorageManager(const string& base_path, int num_subdirs = NUM_SUBFILES);

  virtual ~ObjectStorageManager();

  /// Initialize a ObjectStorageManager.
  Status init();

  // Do nothing.
  Status destory();

  /**
   * \brief Opens a file and creates a new FileObject.
   * \param[in] path not used in object store.
   * \param[in] obj_id object id.
   * \param[in] flags the open flags.
   * \param[out] obj it is filled with new created FileObject's pointer.
   * \return Status::OK if success.
   */
  Status open(const string& path, ObjectId obj_id, int flags, FileObject** obj);

  /**
   * \brief Opens a file with mode and creates a new FileObject.
   * \param[in] path the related path in VSFS.
   * \param[in] flags the open flags.
   * \param[int] mode the mode to open a file.
   * \param[out] obj it is filled with new created FileObject's pointer.
   * \return Status::OK if success.
   */
  Status open(const string& path, ObjectId obj_id, int flags, mode_t mode,
              FileObject** obj);

  /**
   * \brief Reads content from file.
   * \return The size of read content. Return -1 if failed.
   */
  ssize_t read(FileHandler* fh, void* buf, size_t count, off_t offset);

  /**
   * \brief Writes content from file.
   * \return The size of read content. Return -1 if failed.
   */
  ssize_t write(FileHandler* fh, const void* buf, size_t count, off_t offset);

  Status unlink(const string& path, ObjectId obj_id);

  /// Always returns OK.
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
