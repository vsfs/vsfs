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
 * \file file_info.h
 * \brief The presentation of a file object in VSFS.
 */

#ifndef VSFS_COMMON_FILE_INFO_H_
#define VSFS_COMMON_FILE_INFO_H_

#include <stdint.h>
#include <string>

using std::string;

namespace vsfs {

/**
 * \class FileInfo file_info.h "vsfs/common/file_info.h"
 * \brief All information about one File object in VSFS.
 */
class FileInfo {
 public:
  FileInfo();

  /// Copy constructor.
  FileInfo(const FileInfo &rhs) { *this = rhs; }

  /// Assign operator.
  FileInfo& operator=(const FileInfo &rhs);

  FileInfo& operator=(FileInfo&& rhs);

  /// Move constructor.
  FileInfo(FileInfo&&);

  // Not allow to create sub-class.
  ~FileInfo();

  /// Returns the ID of the file.
  uint64_t id() const { return id_; }

  void set_id(uint64_t new_id) { id_ = new_id; }

  /// Returns the path in VSFS namespace.
  const string& path() const { return path_; }

  /// Sets the new VSFS path.
  void set_path(const string &new_path) { path_ = new_path; }

  /// Returns the URI that represents the location of the underlying data
  /// object.
  const string& uri() const { return uri_; }

  /// Sets the new URI of the underlying data storage.
  void set_uri(const string &new_uri) { uri_ = new_uri; }

  /// Returns the UID of the file.
  uint64_t uid() const { return uid_; }

  void set_uid(uint64_t new_user_id) { uid_ = new_user_id; }

  /// Returns the GID of the file.
  uint64_t gid() const { return gid_; }

  void set_gid(uint64_t new_group_id) { gid_ = new_group_id; }

  /// Returns the size of the file.
  uint64_t size() const { return size_; }

  void set_size(uint64_t new_size) { size_ = new_size; }

  /// Returns the mode of the file.
  uint64_t mode() const { return mode_; }

  void set_mode(uint64_t new_mode) { mode_ = new_mode; }

  /// Returns the change time of the file.
  uint64_t ctime() const { return ctime_; }   // NOLINT

  void set_ctime(uint64_t new_ctime) { ctime_ = new_ctime; }

  /// Returns the file modification time of the file.
  uint64_t mtime() const { return mtime_; }

  void set_mtime(uint64_t new_mtime) { mtime_ = new_mtime; }

  /// Returns the acess time of the file.
  uint64_t atime() const { return atime_; }

  void set_atime(uint64_t new_atime) { atime_ = new_atime; }

  /**
   * \brief Sets the file attributes by stating a local file.
   *
   * Note: It dose not set 'id', 'path', 'uri'. You have to manually set them
   * elsewhere.
   */
  int set_from_local_file(const string &local_path);

  /// Sets the stbuf's fields for this file.
  void set_statbuf(struct stat *stbuf) const;

  /// Generates an informative string for debug purpose.
  string to_string() const;

  /// Release the file ID.
  void release();

 private:
  /// A universal ID in VSFS.
  // TODO(lxu): use 128 bits someday..
  uint64_t id_;

  /// User ID.
  uint64_t uid_;

  /// Group ID.
  uint64_t gid_;

  /// File mode.
  uint64_t mode_;

  /// File size.
  uint64_t size_;

  /// File change time.
  uint64_t ctime_;

  /// File modification time.
  uint64_t mtime_;

  /// File access time.
  uint64_t atime_;

  /// The path in VSFS namespace.
  string path_;

  /// The uri for the underlying storage.
  string uri_;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_FILE_INFO_H_
