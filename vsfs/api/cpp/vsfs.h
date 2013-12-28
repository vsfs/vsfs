/*
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

#ifndef VSFS_API_CPP_VSFS_H_
#define VSFS_API_CPP_VSFS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/common/types.h"

namespace vsfs {

class File;
class StorageManager;

namespace client {
class VSFSClient;
}

/**
 * \class Vsfs
 * \brief Vsfs's C++ API.
 */
class Vsfs {
 public:
  // TODO(eddyxu): should move to a public head file
  typedef vsfs::File File;

  enum class KeyType {
    CHAR = TypeIDs::CHAR,
    UINT8 = TypeIDs::UINT8,
    INT8 = TypeIDs::INT8,
    UINT16 = TypeIDs::UINT16,
    INT16 = TypeIDs::INT16,
    UINT32 = TypeIDs::UINT32,
    INT32 = TypeIDs::INT32,
    UINT64 = TypeIDs::UINT64,
    INT64 = TypeIDs::INT64,
    FLOAT = TypeIDs::FLOAT,
    DOUBLE = TypeIDs::DOUBLE,
    STRING = TypeIDs::STRING
  };

  /**
   * \brief Constructs a Vsfs connection with host and port.
   * \param host the hostname of master server.
   * \param port the listen port of master server.
   * \param sm the storage manager instance of master server.
   */
  Vsfs(const std::string& host, int port, StorageManager* sm);

  /**
   * \brief Constructs a Vsfs connection with URI.
   * \param uri the URI of the primary master (e.g., 'vsfs://node1:8829').
   */
  // explicit Vsfs(const std::string& uri);

  /// Used for dependancy injection.
  explicit Vsfs(client::VSFSClient* mock, StorageManager* sm);

  virtual ~Vsfs();

  /// Connect to Vsfs metadata servers.
  vobla::Status connect();

  /// Disconnect from Vsfs metadata servers (e.g., master servers).
  vobla::Status disconnect();

  /// Creates a new file in the VSFS namespace.
  vobla::Status create(const std::string& path, int64_t mode, int64_t uid,
                       int64_t gid, File** fobj);

  /// Deletes a new file.
  vobla::Status unlink(const std::string& path);

  /**
   * \brief Get stat(2) information for a file.
   * \param path the absolute path of file or directory.
   * \param stbuf the stat(2) structure to contain file informations.
   */
  vobla::Status getattr(const std::string& path, struct stat* stbuf);

  /**
   * \brief Creates a new directory on path.
   * \param path the absolute path of directory to be created.
   * \param mode the mode of the new created directory.
   * \param uid the user id of the new created directory.
   * \param gid the group id of the new created directory.
   * \return Status::OK if success.
   */
  vobla::Status mkdir(const std::string& path, int64_t mode,
                      int64_t uid, int64_t gid);

  /**
   * \brief Deletes a directory.
   * \param path the absolute path of the directory to be deleted.
   * \return Status::OK if success.
   *
   * TODO(eddyxu): Add recursive remove directories.
   */
  vobla::Status rmdir(const std::string& path);

  /**
   * \brief Reads a directory and fills `sub_files`.
   * \param root the absolute path of the directory to be iterated.
   * \param sub_files filled with names of sub-files and sub-directories.
   */
  vobla::Status readdir(const std::string& root,  // NOLINT
                        std::vector<std::string>* sub_files);

 private:
  std::unique_ptr<client::VSFSClient> client_;
  StorageManager* storage_manager_;
};

}  // namespace vsfs

#endif  // VSFS_API_CPP_VSFS_H_
