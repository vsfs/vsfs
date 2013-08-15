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

using std::string;
using vobla::Status;

namespace vsfs {

class FileHandler;
class FileObject;

/*
 * \class StorageManager
 * \brief Abstraction and Wrapper for underlying storage system.
 */
class StorageManager : boost::noncopyable {
  public:
    StorageManager() = default;

    virtual ~StorageManager() {}

    /// Initializes a particular storage manager.
    virtual Status init() = 0;

    /// Destroys this storage manager.
    virtual Status destroy() = 0;

    /**
     * \brief open a file according to the logical path and context.
     * \return a new FileObject object, caller now has the ownership of this
     * FileObject object.
     */
    virtual Status open(const string& path, int flags, FileObject** obj) = 0;

    virtual Status open(const string& path, int flags, mode_t mode,
                        FileObject** obj) = 0;

    /// Closes an opened file handler.
    virtual Status close(FileHandler* file_handler) const = 0;

    /**
     * \brief Reads from the file.
     * \return Returns the number of bytes actually read, -1 on error
     */
    virtual ssize_t read(FileHandler* file_handler, void *buf,
                         size_t count, off_t offset) = 0;

    /**
     * \brief Writes to the file.
     * \return Returns the number of bytes actually written, -1 on error
     */
    virtual ssize_t write(FileHandler* file_handler, const void *buf,
                          size_t count, off_t offset) = 0;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_STORAGE_MANAGER_H_
