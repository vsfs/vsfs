/*
 * Copyright 2012 (c) Ziling Huang <hzlgis@gmail.com>
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
#include <string>
#include "vobla/status.h"

using std::string;
using vobla::Status;

namespace vsfs {

class FileHandler;
class FileOp;

/*
 * \class StorageManager
 *
 * \brief Abstraction and Wrapper for underlying filesystem.
 */
class StorageManager : private boost::noncopyable {
  public:
    StorageManager() {}

    virtual ~StorageManager() {}

    /// Initializes a particular storage manager.
    virtual Status init() = 0;

    /// Destroys this storage manager.
    virtual Status destroy() = 0;

    /**
     * \brief open a file according to the logical path and context.
     * \return a new FileOp object, caller now has the ownership of this
     * FileOp object.
     */
    virtual FileOp* open_file(const string &path, int flags) = 0;

    /**
     * \brief close a file
     */
    virtual Status close_file(FileHandler *file_handler) const = 0;

    /**
     * \brief read from the file
     * \return Return the number of bytes actually read, -1 on error
     */
    virtual size_t read(FileHandler *file_handler, void *buf,
                        const size_t count) = 0;

    /**
     * \brief write to the file
     * \return Return the number of bytes actually written, -1 on error
     */
    virtual size_t write(FileHandler *file_handler, const void *buf,
                         const size_t count) = 0;

    /**
     * \brief seek in the file
     * \return Status object
     */
    virtual Status seek(FileHandler *file_handler, const off_t offset,
                        const int whence) const = 0;

    /**
     * \brief flush the data to the disk.
     * \return Status object
     */
    virtual Status flush(FileHandler *file_handler) const = 0;

    /**
     * \brief return the number of opened files in the storage manager.
     */
    virtual int num_opened_files() const = 0;

  protected:
    /*
     * \brief Translate the logical path to physical path in underlying storage
     * \param logical path and context for the file
     * \return physical path for the file
     */
    virtual string translate_path(const string &logical_path) const = 0;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_STORAGE_MANAGER_H_
