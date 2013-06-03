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
 * \file file.cpp
 * \brief The representation of an opened file in VSFS.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <utility>
#include "vsfs/common/file.h"

using std::string;

namespace vsfs {

File::File() {
}

File::File(const char* path, int flags, mode_t mode) {
  fd_ = open(path, flags, mode);
  if (fd_ < 0) {
    // TODO(ziling): handle the open failure here.
  }
}

File::File(const File &rhs) {
  *this = rhs;
}

File::File(File&& other) : fd_(other.fd_) {
  other.release();
}

File& File::operator=(const File &rhs) {
  fd_ = rhs.fd_;
  return *this;
}

File& File::operator=(File&& rhs) {
  fd_ = rhs.fd_;
  return *this;
}

File::~File() {
  close();
}

int File::fd() const {
  return fd_;
}

void File::close() {
  ::close(fd_);
  release();
}

void File::swap(File& other) {
  std::swap(fd_, other.fd_);
}

void File::release() {
  fd_ = -1;
}

}  // namespace vsfs
