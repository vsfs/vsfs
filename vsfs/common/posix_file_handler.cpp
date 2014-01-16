/*
 * Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
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

#include <glog/logging.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "vsfs/common/posix_file_handler.h"

namespace vsfs {

PosixFileHandler::PosixFileHandler(StorageManager* sm, int fd)
    : storage_manager_(sm), fd_(fd) {
  CHECK_NOTNULL(sm);
//  CHECK_GT(fd_, 0);
}

PosixFileHandler::~PosixFileHandler() {
  if (fd_ >= 0) {
    this->close();
  }
}

int PosixFileHandler::fd() const {
  return fd_;
}

Status PosixFileHandler::close() {
  int ret = ::close(fd_);
  if (ret) {
    return Status::system_error(errno);
  }
  fd_ = -1;
  return Status::OK;
}

ssize_t PosixFileHandler::read(void* buf, size_t nbytes, off_t offset) {
  return ::pread(fd_, buf, nbytes, offset);
}

ssize_t PosixFileHandler::write(const void* buf, size_t nbytes, off_t offset) {
  return ::pwrite(fd_, buf, nbytes, offset);
}

}  // namespace vsfs
