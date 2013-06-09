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

#ifndef VSFS_METAD_META_MANAGER_H_
#define VSFS_METAD_META_MANAGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "vobla/status.h"
#include "vsfs/rpc/vsfs_types.h"

using std::mutex;
using std::string;
using std::unordered_map;
using std::vector;
using vobla::Status;
using vsfs::rpc::RpcMetaData;
using vsfs::rpc::RpcMetaDataList;

namespace vsfs {
class LevelDBStore;

namespace metad {

/**
 * \class MetaManager
 * \brief A Key-Value persistent storage for metadata. It handles 1 to 1
 * Mapping from FileID to FilePath.
 *
 * \note This class is thread-safe.
 */
class MetaManager {
 public:
  explicit MetaManager(const string &dbpath);

  explicit MetaManager(vsfs::LevelDBStore* store);

  ~MetaManager();

  /*
   * \brief Loads the data from LevelDBStore.
   */
  Status init();

  /**
   * \brief Inserts a file's id and its path as a record into the key-value
   * storage.
   * \param[in] file_id The unique ID of the file.
   * \param[in] file_path The path of the file.
   */
  Status insert(uint64_t file_id, const string &file_path);

  /**
   * \brief Inserts a bunch of files' id and their path as a record into the
   * key-value storage.
   * \param[in] A vector and (file_id, file_path) pair to be inserted into
   * the key-value storage.
   */
  Status insert(const RpcMetaDataList& metadata);

  /**
   * \brief Removes a file record from the key-value storage.
   * \param[in] file_id the ID of the file.
   */
  Status remove(uint64_t file_id);

  /**
   * \brief Finds a file's path according to its id.
   * \param[in] file_id the ID of the file.
   * \param[out] file_path the path of the file.
   */
  Status find(uint64_t file_id, string *file_path);

  /**
   * \brief Finds a list of file_ids's corresponding file path.
   * \param[in] file_ids the IDs of the files.
   * \param[out] results the corresponding file paths.
   */
  Status search(const vector<int64_t>& file_ids, vector<string>* results);

  /// Gets the number of entries in the key-value storage.
  size_t size();

 private:
  /// Mapping from FileID to FilePath.
  unordered_map<uint64_t, string> map_;

  /// LevelDB store.
  unique_ptr<vsfs::LevelDBStore> store_;

  /// Global lock.
  mutex lock_;
};

}  // namespace metad
}  // namespace vsfs

#endif  // VSFS_METAD_META_MANAGER_H_
