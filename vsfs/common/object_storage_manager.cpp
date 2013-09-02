/*
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

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
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
#include "vsfs/common/object_storage_manager.h"
#include "vsfs/common/posix_file_handler.h"

using std::string;
using std::to_string;
namespace fs = boost::filesystem;

namespace vsfs {

ObjectStorageManager::ObjectStorageManager(const string& base, int nsubdirs)
    : base_path_(base), num_subdirs_(nsubdirs) {
}

ObjectStorageManager::~ObjectStorageManager() {
}

Status ObjectStorageManager::init() {
  // Creates subdirs if not existed.
  for (int i = 0; i < num_subdirs_; i++) {
    auto subdir = fs::path(base_path_) / to_string(i);
    boost::system::error_code ec;
    create_directories(subdir, ec);
    if (ec.value() > 0) {
      return Status(ec.value(), ec.message());
    }
  }
  return Status::OK;
}

Status ObjectStorageManager::destroy() {
  return Status::OK;
}

Status ObjectStorageManager::open(const string&, ObjectId obj_id, int flags,
                                  FileObject** object) {
  CHECK_NOTNULL(object);
  string local_path = translate_path(obj_id);
  int fd = ::open(local_path.c_str(), flags);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open file: " << local_path;
    return Status::system_error(errno);
  }
  *object = new FileObject(new PosixFileHandler(this, fd));
  return Status::OK;
}

Status ObjectStorageManager::open(const string&, ObjectId obj_id, int flags,
                                  mode_t mode, FileObject** obj) {
  CHECK_NOTNULL(obj);
  string local_path = translate_path(obj_id);
  int fd = ::open(local_path.c_str(), flags, mode);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open file: " << local_path;
    return Status::system_error(errno);
  }
  *obj = new FileObject(new PosixFileHandler(this, fd));
  return Status::OK;
}

Status ObjectStorageManager::unlink(const string& path, ObjectId obj_id) {
  string local_path = translate_path(obj_id);
  int ret = ::unlink(local_path.c_str());
  if (ret == -1) {
    LOG(ERROR) << "Failed to unlink " << path << " (Obj: " << obj_id << ")";
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status ObjectStorageManager::mkdir(const string&, mode_t) {
  return Status::OK;
}

Status ObjectStorageManager::rmdir(const string&) {
  return Status::OK;
}

Status ObjectStorageManager::readlink(
    const string&, ObjectId oid, char* buf, size_t size, ssize_t* retlen) {
  string local_path = translate_path(oid);
  *retlen = ::readlink(local_path.c_str(), buf, size);
  if (*retlen < 0) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status ObjectStorageManager::getattr(
    const string&, ObjectId obj_id, struct stat* stbuf) {
  string local_path = translate_path(obj_id);
  int ret = ::stat(local_path.c_str(), stbuf);
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

Status ObjectStorageManager::statfs(struct statvfs* stbuf) {
  CHECK_NOTNULL(stbuf);
  int ret = statvfs(base_path_.c_str(), stbuf);
  if (ret == -1) {
    return Status::system_error(errno);
  }
  return Status::OK;
}

string ObjectStorageManager::translate_path(ObjectId obj_id) const {
  uint64_t subdir_num = static_cast<uint64_t>(obj_id) %
                        static_cast<uint64_t>(num_subdirs_);
  auto path = fs::path(base_path_) / to_string(subdir_num) / to_string(obj_id);
  return path.string();
}

}  // namespace vsfs
