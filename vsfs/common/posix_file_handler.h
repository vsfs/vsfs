/*
 * Copyright 2013-2014 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_COMMON_POSIX_FILE_HANDLER_H_
#define VSFS_COMMON_POSIX_FILE_HANDLER_H_

#include <sys/types.h>
#include "vobla/status.h"
#include "vsfs/common/file_handler.h"

namespace vsfs {

class StorageManager;

/**
 * \class PosixFileHandler
 * \brief Encapsures a file descriptor on UNIX.
 */
class PosixFileHandler : public FileHandler {
 public:
  PosixFileHandler() = delete;

  PosixFileHandler(StorageManager* psm, int fd);

  virtual ~PosixFileHandler();

  Status close();

  ssize_t read(void* buf, size_t nbytes, off_t offset);

  ssize_t write(const void* buf, size_t nbytes, off_t offset);

  int fd() const;

 private:
  StorageManager* storage_manager_;

  int fd_;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_POSIX_FILE_HANDLER_H_
