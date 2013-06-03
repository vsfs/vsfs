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

/**
 * \file file.h
 * \brief The presentation of an opened file in VSFS.
 */

#ifndef VSFS_COMMON_FILE_H_
#define VSFS_COMMON_FILE_H_

#include <algorithm>
#include <string>

using std::string;

namespace vsfs {

/**
  * \class File
  * \brief The representation of an opened file in VSFS.
  */
class File {
 public:
  File();

  /// Constructor.
  File(const char* path, int flags, mode_t mode);

  /// Copy constructor.
  File(const File &rhs);

  /// Move constructor.
  File(File&&);

  /// Assignment operator.
  File& operator=(const File &rhs);

  File& operator=(File&& rhs);

  ~File();

  /// Gets the file descriptor of the file.
  int fd() const;

  /// Close the file and release the file descriptor.
  void close();

  /// Swap another file with this file.
  void swap(File& other);

  /// Release the file descriptor.
  void release();

 private:
  /// File Descriptor.
  int fd_;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_FILE_H_
