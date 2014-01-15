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

#include <boost/utility.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "vobla/status.h"
#include "vobla/traits.h"
#include "vsfs/common/types.h"

using std::pair;
using std::string;
using std::vector;

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
class Vsfs : boost::noncopyable {
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

  Vsfs() = delete;

  /**
   * \brief Constructs a Vsfs connection with host and port.
   * \param host the hostname of master server.
   * \param port the listen port of master server.
   * \param sm the storage manager instance of master server.
   */
  Vsfs(const string& host, int port, StorageManager* sm);

  /**
   * \brief Constructs a Vsfs connection with URI.
   * \param uri the URI of the primary master (e.g., 'vsfs://node1:8829').
   */
  // explicit Vsfs(const string& uri);

  /// Used for dependancy injection.
  explicit Vsfs(client::VSFSClient* mock, StorageManager* sm);

  virtual ~Vsfs();

  /// Connect to Vsfs metadata servers.
  vobla::Status connect();

  /// Disconnect from Vsfs metadata servers (e.g., master servers).
  vobla::Status disconnect();

  /// Creates a new file in the VSFS namespace.
  vobla::Status create(const string& path, int64_t mode, int64_t uid,
                       int64_t gid, File** fobj);

  /// Deletes a new file.
  vobla::Status unlink(const string& path);

  /**
   * \brief Get stat(2) information for a file.
   * \param path the absolute path of file or directory.
   * \param stbuf the stat(2) structure to contain file informations.
   */
  vobla::Status getattr(const string& path, struct stat* stbuf);

  /**
   * \brief Creates a new directory on path.
   * \param path the absolute path of directory to be created.
   * \param mode the mode of the new created directory.
   * \param uid the user id of the new created directory.
   * \param gid the group id of the new created directory.
   * \return Status::OK if success.
   */
  vobla::Status mkdir(const string& path, int64_t mode,
                      int64_t uid, int64_t gid);

  /**
   * \brief Deletes a directory.
   * \param path the absolute path of the directory to be deleted.
   * \return Status::OK if success.
   *
   * TODO(eddyxu): Add recursive remove directories.
   */
  vobla::Status rmdir(const string& path);

  /**
   * \brief Reads a directory and fills `sub_files`.
   * \param root the absolute path of the directory to be iterated.
   * \param sub_files filled with names of sub-files and sub-directories.
   */
  vobla::Status readdir(const string& root,  // NOLINT
                        vector<string>* sub_files);

  /**
   * \brief Creates an named file index on an existing directory.
   * \param path the directoroy path. It must be existed.
   * \param name the name of the index.
   * \param index_type the data structure of index.
   * \param key_type the key type of the index.
   *
   * \note This newly created index has the uid and gid of the caller, and
   * mode of 0777.
   */
  vobla::Status create_index(const string& path,
                             const string& name,
                             int index_type, int key_type);

  /**
   * \brief Creates an named file index on an existing directory.
   * \param path the directoroy path. It must be existed.
   * \param name the name of the index.
   * \param type the string presenting the index type.
   * \param key the string representing the key type of the index.
   *
   * \see create_index() above.
   */
  vobla::Status create_index(const string& path,
                             const string& name,
                             const string& type,
                             const string& key);

  /// Removes a file index.
  vobla::Status destroy_index(const string& path,
                              const string& name);

  /// Pair<file_path, key>
  typedef pair<string, string> FileKeyPair;

  /**
   * \brief insert/update index records to index name.
   * \param name the index name.
   * \param file_key_pairs a vector of (file, key) pairs. The key must be
   * string.
   *
   * Since only the indexd knows the type of a key, so it is sufficient to
   * transfer all index key type as string.
   */
  vobla::Status update_index(const string& name,
                             const vector<FileKeyPair>& file_key_pairs);

  /**
   * \brief A helper function to call the update_index without transforming the
   * keys to strings.
   *
   * It calls the above update_index() internally.
   */
  template <typename Key>
  vobla::Status update_index(const string& name,
                             const vector<pair<string, Key>>& pairs) {
    vector<FileKeyPair> file_str_key_pairs;
    file_str_key_pairs.reserve(pairs.size());
    for (const auto& p : pairs) {
      file_str_key_pairs.emplace_back(
          std::make_pair(p.first, std::to_string(p.second)));
    }
    return update_index(name, file_str_key_pairs);
  }

  /**
   * \brief Removes records from index.
   */
  vobla::Status remove_index(const string& name,
                             const vector<FileKeyPair>& file_key_pairs);

  // TODO(eddyxu): use Cursor or iterator.
  /// Search files.
  vobla::Status search(const string& query, vector<string>* results);

 private:
  std::unique_ptr<client::VSFSClient> client_;
  StorageManager* storage_manager_;
};

}  // namespace vsfs

#endif  // VSFS_API_CPP_VSFS_H_
