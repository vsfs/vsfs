/*
 * Copyright 2013 (c) Ziling Huang <hzlgis@gmail.com>
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
/*
 * \file storage_manager.h
 * \brief Storage Manager, providing an abstract interface for storage
 * pool.
 */
#ifndef VSFS_COMMON_STORAGE_MANAGER_H_
#define VSFS_COMMON_STORAGE_MANAGER_H_

#include <boost/noncopyable.hpp>
#include <sys/types.h>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/types.h"

using std::string;
using vobla::Status;

namespace vsfs {

class FileHandler;
class FileObject;

/*
 * \class StorageManager
 * \brief Abstraction of underlying storage system.
 */
class StorageManager : boost::noncopyable {
  public:
    virtual ~StorageManager() {}

    /// Initializes a particular storage manager.
    virtual Status init() = 0;

    /// Destroys this storage manager.
    virtual Status destroy() = 0;

    /**
     * \brief Opens a file and creates a new FileObject.
     * \param[in] path the related path in VSFS.
     * \param[in] obj_id the object id of the given file.
     * \param[in] flags the open flags.
     * \param[out] obj it is filled with new created FileObject's pointer.
     * \return Status::OK if success.
     */
    virtual Status open(const string& path, ObjectId obj_id, int flags,
                        FileObject** obj) = 0;

    /**
     * \brief Opens a file with mode and creates a new FileObject.
     * \param[in] path the related path in VSFS.
     * \param[in] obj_id the object id of the opening file.
     * \param[in] flags the open flags.
     * \param[int] mode the mode to open a file.
     * \param[out] obj it is filled with new created FileObject's pointer.
     * \return Status::OK if success.
     */
    virtual Status open(const string& path, ObjectId obj_id, int flags,
                        mode_t mode, FileObject** obj) = 0;

    /// Removes the object/file from the storage.
    virtual Status unlink(const string& path, ObjectId obj_id) = 0;

    /// Creates a directory if the underlying storage supports the directlry
    /// semantics.
    virtual Status mkdir(const string& path, mode_t mode) = 0;

    /// Removes a directory if the storage manager supports.
    virtual Status rmdir(const string& path) = 0;

    virtual Status readlink(const string& path, ObjectId obj_id,
                            char* buf, size_t size, ssize_t* retlen) = 0;

    /// \TODO(eddyxu): It is a temporary to get inode information from file
    // system. Will change to query the master server directly.
    virtual Status getattr(const string& path, ObjectId obj_id,
                           struct stat* stbuf) = 0;

    virtual Status statfs(struct statvfs* stbuf) = 0;

  protected:
    // Do not allow to intialize StorageManager directly.
    StorageManager() = default;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_STORAGE_MANAGER_H_
