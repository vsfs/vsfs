/*
 * Copyright 2011-2013 (c) Lei Xu <eddyxu@gmail.com>
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
 * \file posix_file_handler.cpp
 *
 * \brief Implementation of PosixFileHandler.
 */

#include "vsfs/common/posix_file_handler.h"
#include <glog/logging.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

using std::string;

namespace vsfs {

PosixFileHandler::PosixFileHandler(PosixStorageManager *psm, int fd)
    : storage_manager_(psm), fd_(fd) {
  CHECK_NOTNULL(psm);
  CHECK_GT(fd_, 0);
}

PosixFileHandler::~PosixFileHandler() {
  if (fd_) {
    this->close();
  }
}

Status PosixFileHandler::close() {
  int ret = 0;
  ret = ::close(fd_);
  if (ret) {
    return Status(ret, strerror(ret));
  }
  fd_ = 0;
  return Status::OK;
}

size_t PosixFileHandler::read(void *buf, size_t nbytes) {
  return ::read(fd_, buf, nbytes);
}

size_t PosixFileHandler::pread(void *buf, size_t nbytes, off_t offset) {
  return ::pread(fd_, buf, nbytes, offset);
}

size_t PosixFileHandler::write(const void *buf, size_t nbytes) {
  return ::write(fd_, buf, nbytes);
}

size_t PosixFileHandler::pwrite(const void *buf, size_t nbytes, off_t offset) {
  return ::pwrite(fd_, buf, nbytes, offset);
}

Status PosixFileHandler::seek(off_t offset, int whence) {
  int ret = ::lseek(fd_, offset, whence);
  if (!ret) {
    const string errmsg = strerror(errno);
    LOG(ERROR) << "Failed to seek: " << errmsg;
    return Status(ret, errmsg);
  }
  return Status::OK;
}

Status PosixFileHandler::flush() {
  return Status::OK;
}
}  // namespace vsfs
