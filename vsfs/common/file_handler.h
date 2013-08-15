/**
 * Copyright (c) 2013 Ziling Huang <hzlgis@gmail.com>
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

#ifndef VSFS_COMMON_FILE_HANDLER_H_
#define VSFS_COMMON_FILE_HANDLER_H_

#include <boost/noncopyable.hpp>
#include <sys/types.h>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/types.h"

using std::string;
using vobla::Status;

namespace vsfs {

/**
 * \class FileHandler
 * \breif Base Class of File Handler, a class to store File Infomation such
 * as FileHandle, Local File Path etc. It also stores a ptr to its storage
 * manager so that it can call the file operation through that ptr. File
 * Handler has a one to one relationship with FilOp class, each FileOp
 * instance contain one unique File Handler instance.
 *
 */
class FileHandler : boost::noncopyable {
 public:
  FileHandler() = default;

  virtual ~FileHandler() {}

  /// Returns the ID for this file handler.
  virtual ObjectId object_id() const = 0;

  /// close the file and return Status object.
  virtual Status close() = 0;

  /// Reads from the file with offset.
  virtual ssize_t read(void *buf, size_t nbytes, off_t offset) = 0;

  /// Writes nbytes to the file.
  virtual ssize_t write(const void *buf, size_t nbytes, off_t offset) = 0;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_FILE_HANDLER_H_
