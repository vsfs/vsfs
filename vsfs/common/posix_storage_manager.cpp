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
 * \brief Implementation of the PosixStorageManager.
 */

#include <fcntl.h>
#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/file_object.h"
#include "vsfs/common/posix_file_handler.h"
#include "vsfs/common/posix_storage_manager.h"

using std::unique_ptr;
using std::string;
using vobla::Status;

namespace vsfs {

PosixStorageManager::PosixStorageManager(const string& base_path)
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

Status PosixStorageManager::open(
    const string& path, ObjectId, int flags, FileObject** obj) {
  CHECK_NOTNULL(obj);
  const string local_path = translate_path(path);
  int fd = ::open(local_path.c_str(), flags);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open file: " << local_path;
    return Status::system_error(errno);
  }
  *obj = new FileObject(new PosixFileHandler(this, fd));
  return Status::OK;
}

Status PosixStorageManager::open(
    const string& path, ObjectId, int flags, mode_t mode, FileObject** obj) {
  CHECK_NOTNULL(obj);
  string local_path = translate_path(path);
  int fd = ::open(local_path.c_str(), flags, mode);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open file: " << local_path;
    return Status::system_error(errno);
  }
  *obj = new FileObject(new PosixFileHandler(this, fd));
  return Status::OK;
}

Status PosixStorageManager::unlink(const string& path, ObjectId) {
  string local_path = translate_path(path);
  int ret = ::unlink(local_path.c_str());
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status PosixStorageManager::mkdir(const string& path, mode_t mode) {
  string local_path = translate_path(path);
  int ret = ::mkdir(local_path.c_str(), mode);
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status PosixStorageManager::rmdir(const string& path) {
  string local_path = translate_path(path);
  int ret = ::rmdir(local_path.c_str());
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status PosixStorageManager::readlink(
    const string& path, ObjectId, char* buf, size_t size, ssize_t* retlen) {
  string local_path = translate_path(path);
  *retlen = ::readlink(local_path.c_str(), buf, size);
  if (*retlen < 0) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status PosixStorageManager::getattr(
    const string& path, ObjectId, struct stat* stbuf) {
  string local_path = translate_path(path);
  int ret = ::stat(local_path.c_str(), stbuf);
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status PosixStorageManager::statfs(struct statvfs* stbuf) {
  CHECK_NOTNULL(stbuf);
  int ret = statvfs(base_path_.c_str(), stbuf);
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status PosixStorageManager::symlink(const string& fpath,
                                    const string& link_path, ObjectId) {
  string local_path = translate_path(link_path);
  int ret = ::symlink(fpath.c_str(), local_path.c_str());
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

string PosixStorageManager::translate_path(const string& path) const {
  return base_path_ + path;
}

}  // namespace vsfs
