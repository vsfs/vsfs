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

#include <boost/noncopyable.hpp>
#include <sys/types.h>
#include <memory>
#include <string>

using std::string;
using std::unique_ptr;

namespace vobla {
class Status;
}

namespace vsfs {

class FileHandler;

/**
 * \class FileObject file_op.h
 * \brief file operation wrapper for storage manager. It stores a file handler
 * of the file. Each FileObject instance has a one to one relationship with its
 * file handler.
 */
class FileObject : private boost::noncopyable {
 public:
  FileObject() = delete;

  explicit FileObject(FileHandler* file_handler);

  /// Move constructor.
  FileObject(FileObject&& rhs);

  ~FileObject();

  /**
   * \brief Reads data starting from an offset.
   * \param buf the output buffer for read operation.
   * \param count the number of bytes to read.
   * \param offset the offset from the file begining to read.
   * \return the actural bytes read.
   */
  ssize_t read(void *buf, size_t count, off_t offset);

  /**
   * \brief Writes the buffer from the offset of file.
   * \see pread()
   */
  ssize_t write(const void *buf, size_t count, off_t offset);

  /// Closes the file it holds.
  vobla::Status close();

  /// Returns the pointer of the underlying file handler.
  FileHandler* file_handler();

 private:
  // Pointer to File Handler
  unique_ptr<FileHandler> file_handler_;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_FILE_OBJECT_H_
