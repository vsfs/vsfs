/**
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
#ifndef VSFS_COMMON_FILE_OBJECT_H_
#define VSFS_COMMON_FILE_OBJECT_H_

#include <glog/logging.h>
#include <sys/types.h>
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>
#include "vsfs/common/file_handler.h"

using std::string;
using std::unique_ptr;

namespace vsfs {

/**
 * \class FileObject file_op.h
 * \brief file operation wrapper for storage manager. It stores a file handler
 * of the file. Each FileObject instance has a one to one relationship with its
 * file handler.
 */
class FileObject : private boost::noncopyable {
 public:
  explicit FileObject(FileHandler *file_handler) : file_handler_(file_handler) {
    CHECK_NOTNULL(file_handler);
  }

  ~FileObject() {}

  /**
   * \brief Reads data starting from an offset.
   * \param buf the output buffer for read operation.
   * \param count the number of bytes to read.
   * \param offset the offset from the file begining to read.
   * \return the actural bytes read.
   */
  size_t read(void *buf, size_t count, off_t offset) {
    return file_handler_->read(buf, count, offset);
  }

  /**
   * \brief Writes the buffer from the offset of file.
   * \see pread()
   */
  size_t write(const void *buf, size_t count, off_t offset) {
    return file_handler_->write(buf, count, offset);
  }

  /**
   * \brief close the file
   */
  Status close() {
    return get_file_handler()->close();
  }

  FileHandler* get_file_handler() {
    return file_handler_.get();
  }

 private:
  // Pointer to File Handler
  unique_ptr<FileHandler> file_handler_;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_FILE_OBJECT_H_
