/**
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
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
#include "vsfs/common/file_handler.h"
#include "vsfs/common/file_object.h"

using vobla::Status;

namespace vsfs {

FileObject::FileObject(FileHandler* fh) : file_handler_(fh) {
  CHECK_NOTNULL(fh);
}

FileObject::FileObject(FileObject&& rhs) {
  file_handler_ = std::move(rhs.file_handler_);
}

FileObject::~FileObject() {
  if (file_handler_) {
    file_handler_->close();
  }
}

ssize_t FileObject::read(void *buf, size_t count, off_t offset) {
  return file_handler_->read(buf, count, offset);
}

ssize_t FileObject::write(const void *buf, size_t count, off_t offset) {
  return file_handler_->write(buf, count, offset);
}

Status FileObject::close() {
  auto status = file_handler_->close();
  if (status.ok()) {
    file_handler_.reset();
  }
  return status;
}

FileHandler* FileObject::file_handler() const {
  return file_handler_.get();
}

int FileObject::fd() const {
  return file_handler_->fd();
}

}  // namespace vsfs
