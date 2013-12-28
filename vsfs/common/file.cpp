/**
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
#include "vobla/status.h"
#include "vsfs/common/file.h"
#include "vsfs/common/file_handler.h"

using vobla::Status;

namespace vsfs {

File::File(FileHandler* fh) : file_handler_(fh) {
  CHECK_NOTNULL(fh);
}

File::File(File&& rhs) {
  file_handler_ = std::move(rhs.file_handler_);
}

File::~File() {
  if (file_handler_) {
    file_handler_->close();
  }
}

ssize_t File::read(void *buf, size_t count, off_t offset) {
  return file_handler_->read(buf, count, offset);
}

ssize_t File::write(const void *buf, size_t count, off_t offset) {
  return file_handler_->write(buf, count, offset);
}

Status File::close() {
  auto status = file_handler_->close();
  if (status.ok()) {
    file_handler_.reset();
  }
  return status;
}

FileHandler* File::file_handler() const {
  return file_handler_.get();
}

int File::fd() const {
  return file_handler_->fd();
}

}  // namespace vsfs
