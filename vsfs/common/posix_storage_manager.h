/*
 * Copyright 2011-2013 (c) Lei Xu <eddyxu@gmail.com>
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

/**
 * \file posix_storage_manager.h
 * \brief A StorageManager runs on top of POSIX file interface.
 *
 * This storage manager is used to run on top of native fs (e.g., Ext4/Btrfs)
 * or networked file system (e.g., Lustra/NFS).
 */

#ifndef VSFS_COMMON_POSIX_STORAGE_MANAGER_H_
#define VSFS_COMMON_POSIX_STORAGE_MANAGER_H_

#include <string>
#include "vsfs/common/storage_manager.h"

using std::string;

namespace vsfs {

class FileObject;

/**
 * \class PosixStorageManager
 * \brief StorageManager runs ontop of any file systems supporting POSIX APIs.
 */
class PosixStorageManager : public StorageManager {
 public:
  PosixStorageManager() = delete;
  explicit PosixStorageManager(const string &base_path);

  virtual ~PosixStorageManager();

  /**
   * \brief Initializes this PosixStorageManager.
   * \return It always returns Status::OK.
   */
  Status init();

  /**
   * \brief Destroys this storage manager.
   * \return always returns Status::OK, because it does nothing.
   */
  Status destroy();

  Status open(const string& path, int flags, FileObject** obj);

  Status open(const string& path, int flags, mode_t mode,
              FileObject** obj);

  Status close(FileHandler* handler) const;

  ssize_t read(FileHandler *file_handler, void *buf, size_t count,
               off_t offset);

  ssize_t write(FileHandler *file_handler, const void *buf,
                size_t count, off_t offset);

 private:
  /// Returns the native path of the file requested.
  string translate_path(const string &path) const;

  string base_path_;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_POSIX_STORAGE_MANAGER_H_
