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

/**
 * \file file_handler.h
 * \breif Base Class of File Handler, a class to store File Infomation such
 * as FileHandle, Local File Path etc. It also stores a ptr to its storage
 * manager so that it can call the file operation through that ptr. File
 * Handler has a one to one relationship with FilOp class, each FileOp
 * instance contain one unique File Handler instance.
 *
 */

#ifndef VSFS_COMMON_FILE_HANDLER_H_
#define VSFS_COMMON_FILE_HANDLER_H_

#include <sys/types.h>
#include <string>
#include "vobla/macros.h"
#include "vobla/status.h"

using std::string;
using vobla::Status;

namespace vsfs {

/**
 * \class FileHandler
 * \brief Basic class of FileHandler, which conceptually equals to the file
 * descriptor.
 */
class FileHandler {
 public:
  FileHandler() = default;

  virtual ~FileHandler() {}

  /// Returns the ID for this file handler.
  virtual int id() const = 0;

  /// close the file and return Status object.
  virtual Status close() = 0;

  /// read from the file
  virtual size_t read(void *buf, size_t nbytes) = 0;

  /// Reads from the file with offset.
  virtual size_t pread(void *buf, size_t nbytes, off_t offset) = 0;

  /// write to the file.
  virtual size_t write(const void *buf, size_t nbytes) = 0;

  /// Writes nbytes to the file.
  virtual size_t pwrite(const void *buf, size_t nbytes, off_t offset) = 0;

  /// seek in the file
  virtual Status seek(off_t offset, int whence) = 0;

  virtual Status flush() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileHandler);
};

}  // namespace vsfs

#endif  // VSFS_COMMON_FILE_HANDLER_H_
