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

#ifndef VSFS_MASTERD_MASTER_CONTROLLER_H_
#define VSFS_MASTERD_MASTER_CONTROLLER_H_

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <gtest/gtest_prod.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "vsfs/common/types.h"
#include "vobla/macros.h"
#include "vobla/status.h"
#include "vsfs/rpc/vsfs_types.h"

namespace apache { namespace thrift { namespace server {
class TServer;
} } }

using apache::thrift::server::TServer;
using boost::shared_ptr;
using std::condition_variable;
using std::mutex;
using std::string;
using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::Status;

namespace vsfs {

class ServerMap;

namespace index {
class IndexInfo;
}

namespace masterd {

class IndexNamespaceInterface;
class Namespace;
class PartitionManagerInterface;

/**
 * \brief MasterController the main control block of master server.
 *
 * It maintains the following mappings.
 *  - Index name -> Index server
 *  - Index server -> Its IP address (addr:port)
 *  - Index Info -> Index Name
 */
class MasterController : boost::noncopyable {
 public:
  /// Default constructor.
  MasterController() = delete;

  /**
   * \brief Constructs a MasterController with given directory.
   * \param basedir the basedir to store metadata.
   * \param host the host name of this master node.
   * \param port the port to listen.
   * \param primary Sets it to true if this masterd is the primary masterd.
   */
  explicit MasterController(const string& basedir,
                            const string& host = "",
                            int port = 9876,
                            bool primary = false,
                            const string& primary_host = "",
                            int primary_port = 9876);

  /**
   * \brief Constructs a MasterController using dependency injections.
   *
   * It must be used by tests.
   */
  MasterController(IndexNamespaceInterface* idx_ns,
                   PartitionManagerInterface* pm,
                   const string& host = "",
                   int port = 9876,
                   bool primary = false);

  virtual ~MasterController();

  /// Initialize the namespace and index partitions.
  Status init();

  /// Starts the Masterd RPC server.
  void start();

  /// Stops the Masterd RPC server.
  void stop();

  /**
   * \brief A MasterServer asks to join the consistent ring.
   * \note There is no replica server needed for master node (for now).
   */
  Status join_master_server(const NodeInfo& node);

  /**
   * \brief Lets an IndexServer join the consistent hash ring, and
   * send the address of its replica machines back to it.
   *
   * \param node The node information of the joining index server.
   * \param replicas A node address vector that is filled with the addresses
   * for the replica servers.
   * \return Status::OK if success.
   */
  Status join_index_server(const NodeInfo &node, RpcNodeAddressList *replicas);

  /**
   * \brief Returns all the master node address.
   * \return Status(-1, ..) if this is not the primary master.
   */
  Status get_all_masters(RpcConsistentHashRing* ring);

  /**
   * \brief Returns C.H ring of index server.
   * \param[out] ring filled with the index server ring.
   * \return Status(-1, ..) if this is not the primary master.
   */
  Status get_all_index_servers(RpcConsistentHashRing* ring);

  Status mkdir(const string& path, mode_t mode, uid_t uid, gid_t gid);

  Status rmdir(const string& path);

  /**
   * \brief Adds a file name into a directory structure.
   * \param parent the path of parent directory.
   * \param subfile the filename of the inserted file.
   *
   * \note It does not guarentee the atomic of file creating operation, which
   * should be taking cared by the client.
   */
  Status add_subfile(const string& parent, const string& subfile);

  Status remove_subfile(const string& parent, const string& subfile);

  Status readdir(const string& path, vector<string>* subfiles);  // NOLINT

  /// Creates a new file and sets its metadata.
  Status create(const string& path, int mode, uid_t uid, gid_t gid,
                ObjectId* oid);

  /// Removes a file if exists.
  Status remove(const string& path);

  /// Gets the object ID for the path, if existed.
  Status object_id(const string& path, ObjectId* oid);

  /// Gets the attribute of a file / dir / index.
  Status getattr(const string &path, RpcFileInfo *info);

  void find_objects(const vector<string>& files, vector<ObjectId>* objects);

  Status find_files(const vector<ObjectId>& objects, vector<string>* files);

  /**
   * \brief Creates an File Index and returns the index server locations.
   * \param[in] request the index creation request.
   * \return Status::OK if success.
   */
  Status create_index(const RpcIndexCreateRequest& request);

  /**
   * \brief Removes a file index.
   */
  Status remove_index(const string& root, const string& name);

  /**
   * \brief Locates all indices with `names` that have files under
   * `root` path, for search request.
   */
  Status locate_indices(const string& root, const vector<string>& names,
                        vector<string>* indices);

  /// Finds all indices with any names under the directory 'root'
  vector<string> locate_indices(const string& root, bool recursive);

  string host() const;

  int port() const;

 private:
  FRIEND_TEST(MasterControllerTest, TestCreateIndex);

  enum class RuntimeStatus {
    PREPARE, RUNNING, STOPPED
  };

  void background_task();

  shared_ptr<TServer> server_;

  /// Host name of this master server.
  string host_;

  /// Listening port of this master server.
  int port_;

  /// Sets to true if this master is a primary node.
  bool is_primary_node_;

  /// The hostname of the primary node (Optional).
  string primary_host_;

  /// The port of the primary node.
  int primary_port_;

  /// The thread to run 'background_task()'
  thread background_thread_;

  condition_variable background_cv_;

  mutex background_mutex_;

  RuntimeStatus runtime_status_;

  unique_ptr<IndexNamespaceInterface> index_namespace_;

  unique_ptr<PartitionManagerInterface> index_partition_manager_;

  unique_ptr<ServerMap> master_server_manager_;

  unique_ptr<ServerMap> index_server_manager_;

  unique_ptr<Namespace> namespace_;
};

}  // namespace masterd
}  // namespace vsfs
#endif  // VSFS_MASTERD_MASTER_CONTROLLER_H_
