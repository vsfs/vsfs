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
 * \file posix_storage_manager.cpp
 *
 * \brief Implementation of the PosixStorageManager.
 */

#include <fcntl.h>
#include <glog/logging.h>
#include <memory>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/file_op.h"
#include "vsfs/common/posix_file_handler.h"
#include "vsfs/common/posix_storage_manager.h"

using std::unique_ptr;
using std::string;

namespace vsfs {

PosixStorageManager::PosixStorageManager(const string &base_path)
    : base_path_(base_path) {
}

PosixStorageManager::~PosixStorageManager() {
}

Status PosixStorageManager::init() {
  return Status::OK;
}

Status PosixStorageManager::destroy() {
  return Status::OK;
}

FileOp* PosixStorageManager::open_file(const string &path, int flags) {
  const string local_path = translate_path(path);
  int fd = open(path.c_str(), flags);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open file: " << path;
    return nullptr;
  }
  unique_ptr<PosixFileHandler> file_handler(new PosixFileHandler(this, fd));
  return new FileOp(file_handler.release());
}

Status PosixStorageManager::close_file(vsfs::FileHandler* handler) const {
  CHECK_NOTNULL(handler);
  return handler->close();
}

size_t PosixStorageManager::read(FileHandler *handler, void *buf,
                                 const size_t count) {
  CHECK_NOTNULL(handler);
  return handler->read(buf, count);
}

size_t PosixStorageManager::write(FileHandler *handler, const void *buf,
                                  const size_t count) {
  CHECK_NOTNULL(handler);
  CHECK_NOTNULL(buf);
  return handler->write(buf, count);
}

Status PosixStorageManager::seek(FileHandler *handler, const off_t offset,
                                 const int whence) const {
  CHECK_NOTNULL(handler);
  return handler->seek(offset, whence);
}

Status PosixStorageManager::flush(FileHandler *handler) const {
  return handler->flush();
}

int PosixStorageManager::num_opened_files() const {
  return 0;
}

string PosixStorageManager::translate_path(const string &path) const {
  return base_path_ + "/" + path;
}

}  // namespace vsfs
