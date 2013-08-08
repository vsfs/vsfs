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

PosixFileHandler::PosixFileHandler(PosixStorageManager *psm, ObjectId objectId)
    : storage_manager_(psm), objectId_(objectId) {
  CHECK_NOTNULL(psm);
  CHECK_GT(objectId_, 0);
}

PosixFileHandler::~PosixFileHandler() {
  if (objectId_) {
    this->close();
  }
}

Status PosixFileHandler::close() {
  int ret = 0;
  ret = ::close(objectId_);
  if (ret) {
    return Status(ret, strerror(ret));
  }
  objectId_ = 0;
  return Status::OK;
}

ssize_t PosixFileHandler::read(void *buf, size_t nbytes, off_t offset) {
  return ::pread(objectId_, buf, nbytes, offset);
}

ssize_t PosixFileHandler::write(const void *buf, size_t nbytes, off_t offset) {
  return ::pwrite(objectId_, buf, nbytes, offset);
}

}  // namespace vsfs