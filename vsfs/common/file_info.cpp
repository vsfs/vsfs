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

/**
 * \file file_info.cpp
 * \brief Implementation of file object information.
 */

#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include "vsfs/common/file_info.h"

namespace vsfs {

FileInfo::FileInfo() : id_(0), uid_(0), gid_(0), mode_(0), size_(0),
    ctime_(0), mtime_(0), atime_(0) {
}

FileInfo::~FileInfo() {
}

FileInfo& FileInfo::operator=(const FileInfo &rhs) {
  id_ = rhs.id_;
  uid_ = rhs.uid_;
  gid_ = rhs.gid_;
  mode_ = rhs.mode_;
  size_ = rhs.size_;
  ctime_ = rhs.ctime_;
  mtime_ = rhs.mtime_;
  atime_ = rhs.atime_;
  path_ = rhs.path_;
  uri_ = rhs.uri_;
  return *this;
}

FileInfo& FileInfo::operator=(FileInfo&& rhs) {
  id_ = rhs.id_;
  uid_ = rhs.uid_;
  gid_ = rhs.gid_;
  mode_ = rhs.mode_;
  size_ = rhs.size_;
  ctime_ = rhs.ctime_;
  mtime_ = rhs.mtime_;
  atime_ = rhs.atime_;
  path_ = rhs.path_;
  uri_ = rhs.uri_;
  return *this;
}

FileInfo::FileInfo(FileInfo&& other)
  : id_(other.id_)
  , uid_(other.uid_)
  , gid_(other.gid_)
  , mode_(other.mode_)
  , size_(other.size_)
  , ctime_(other.ctime_)
  , mtime_(other.mtime_)
  , atime_(other.atime_)
  , path_(other.path_)
  , uri_(other.uri_) {
  other.release();
}

int FileInfo::set_from_local_file(const string &local_path) {
  int ret = 0;
  struct stat stbuf;
  ret = stat(local_path.c_str(), &stbuf);
  if (ret) {
    return ret;
  }
  uid_ = stbuf.st_uid;
  gid_ = stbuf.st_gid;
  mode_ = stbuf.st_mode;
  size_ = stbuf.st_size;
  atime_ = stbuf.st_atime;
  ctime_ = stbuf.st_ctime;
  mtime_ = stbuf.st_mtime;

  return ret;
}

void FileInfo::set_statbuf(struct stat* stbuf) const {
  CHECK_NOTNULL(stbuf);
  stbuf->st_uid = uid();
  stbuf->st_gid = gid();
  stbuf->st_size = size();
  stbuf->st_mode = mode();
  stbuf->st_atime = atime();
  stbuf->st_mtime = mtime();
  stbuf->st_ctime = ctime();  // NOLINT
}

void FileInfo::release() {
  id_ = 0;
  uid_ = 0;
  gid_ = 0;
  mode_ = 0;
  size_ = 0;
  atime_ = 0;
  ctime_ = 0;
  mtime_ = 0;
  path_ = "";
  uri_ = "";
}

}  // namespace vsfs
