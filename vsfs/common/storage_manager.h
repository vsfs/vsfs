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
    virtual Status open(const string& path, ObjectId obj_id, int flags,
                        FileObject** obj) = 0;

    virtual Status open(const string& path, ObjectId obj_id, int flags,
                        mode_t mode, FileObject** obj) = 0;

    virtual Status mkdir(const string& path, mode_t mode) = 0;

    virtual Status rmdir(const string& path) = 0;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_STORAGE_MANAGER_H_
