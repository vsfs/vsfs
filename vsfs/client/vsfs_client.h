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

#ifndef VSFS_CLIENT_VSFS_CLIENT_H_
#define VSFS_CLIENT_VSFS_CLIENT_H_

#include <boost/utility.hpp>
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vsfs/common/types.h"

using std::string;
using std::unique_ptr;
using std::vector;
using vobla::Status;

namespace vsfs {

class ComplexQuery;

namespace index {
class IndexInfo;
}

namespace client {

/**
 * \class VSFSClient.
 * \brief The client interface to contact with a VSFS cluster.
 *
 * The basic usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 *   unique_ptr<VSFSClient> client(
 *      VSFSClient::CreateVSFSRpcClient(host, port));
 *   Status status = client->init();
 *   if (status.ok()) {
 *      VSFSClient::FileInfo file;
 *      status = client->open("/foo/bar.txt", &file);
 *      if (!status.ok()) {
 *         // error handling.
 *      }
 *      const string &loc = file.location();
 *      // if it is a HDFS file.
 *      hdfs_open(loc, mode);
 *      ...
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 */
class VSFSClient : boost::noncopyable {
 public:
  // Factory methods.
  /**
   * \brief Creates a VSFS Rpc client.
   * \param host the host name of master node.
   * \param int the port number of master node.
   * \return a pointer of newly allocated VSFSClient object and transfers its
   * ownership to the caller.
   *
   * \note This function does not connect the VSFSRpcClient to the Master Node.
   * You must call init() to establish the connection.
   */
  static VSFSClient* CreateVSFSRpcClient(const string &host, int port);

  virtual ~VSFSClient() {}

  /// Initializes this client for the sub-class.
  virtual Status init() = 0;

  /**
   * \brief Creates a new directory on path.
   * \param path the absolute path of directory to be created.
   * \param mode the mode of the new created directory.
   * \param uid the user id of the new created directory.
   * \param gid the group id of the new created directory.
   * \return Status::OK if success.
   */
  virtual Status mkdir(const string& path, int64_t mode,
                       int64_t uid, int64_t gid) = 0;

  /**
   * \brief Deletes a directory.
   * \param path the absolute path of the directory to be deleted.
   * \return Status::OK if success
   */
  virtual Status rmdir(const string& path) = 0;

  /// Reads all files from one directory.
  virtual Status readdir(const string& dir, vector<string>* subfiles) = 0;  // NOLINT

  /// Creates a new file in the VSFS namespace.
  virtual Status create(const string& path, int64_t mode, int64_t uid,
                        int64_t gid, ObjectId* id) = 0;

  /// Opens a file and get the file handler.
  virtual Status open(const string& path, ObjectId* id) = 0;

  /// Removes a file and returns its ObjectId.
  virtual Status unlink(const string& path, ObjectId* id) = 0;

  virtual Status create_index(
      const string& root, const string& name, int index_type, int key_type,
      int64_t mode, int64_t uid, int64_t gid) = 0;

  virtual Status search(const ComplexQuery& query,
                        vector<string>* results) = 0;

  /// Encapsulate the request to updating an index record.
  struct IndexUpdateRequest {
    enum { UNKNOWN, INSERT, UPDATE, REMOVE };

    IndexUpdateRequest() {
    }

    IndexUpdateRequest(
        int opcode, const string& path, const string& name, const string& k)
        : op(opcode), file_path(path), index_name(name), key(k) {
    }

    IndexUpdateRequest(const IndexUpdateRequest& rhs) {
      op = rhs.op;
      file_path = rhs.file_path;
      index_name = rhs.index_name;
      key = rhs.key;
    }

    int op;
    string file_path;
    string index_name;
    string key;
  };

  /**
   * \brief Applies a sequence of index update requests to the VSFS
   * \param updates a time-ordered sequence of index updates.
   */
  virtual Status update(const vector<IndexUpdateRequest>& updates) = 0;

  /**
   * \brief Gets the information for all indices on the path.
   * \param[in] path The path to find indices.
   * \param[out] infos Filled with all index infos on this path.
   */
  virtual Status info(const string& path,
                      vector<index::IndexInfo>* infos) = 0;

  /**
   * \brief Get the metadata for indices on directory 'path'.
   * \param[in] path the path of a directory.
   * \param[out] infos filled with the index metadata on the directory.
   * \param[in] recusive sets to true to fetch index informations recursively.
   */
  virtual Status get_index_infos(const string& path,
                                 vector<index::IndexInfo>* infos,
                                 bool recursive = false) = 0;
};

}  // namespace client
}  // namespace vsfs

#endif  // VSFS_CLIENT_VSFS_CLIENT_H_
