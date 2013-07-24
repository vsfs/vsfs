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

#include <boost/shared_ptr.hpp>
#include <gtest/gtest_prod.h>
#include <memory>
#include <string>
#include <vector>
#include "vobla/macros.h"
#include "vobla/status.h"
#include "vsfs/rpc/vsfs_types.h"

namespace apache { namespace thrift { namespace server {
class TServer;
} } }

using apache::thrift::server::TServer;
using boost::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using vobla::Status;

namespace vsfs {

namespace index {
class IndexInfo;
}

namespace masterd {

class IndexNamespaceInterface;
class Namespace;
class PartitionManagerInterface;
class ServerManager;

/**
 * \brief MasterController the main control block of master server.
 *
 * It maintains the following mappings.
 *  - Index name -> Index server
 *  - Index server -> Its IP address (addr:port)
 *  - Index Info -> Index Name
 */
class MasterController {
 public:
  /// Default constructor.
  MasterController();

  /**
   * \brief Constructs a MasterController with given directory.
   *
   * The namespace and index partitons will be stored in the 'basedir'.
   */
  explicit MasterController(const string& basedir);

  /**
   * \brief Constructs a MasterController using dependency injections.
   *
   * It must be used by tests.
   */
  MasterController(IndexNamespaceInterface* idx_ns,
                   PartitionManagerInterface* pm);

  virtual ~MasterController();

  /// Initialize the namespace and index partitions.
  Status init();

  /// Starts the Masterd RPC server.
  void start();

  /// Stops the Masterd RPC server.
  void stop();

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

  Status mkdir(const string& path, mode_t mode, uid_t uid, gid_t gid);

  Status rmdir(const string& path);

  Status readdir(const string& path, vector<string>* subfiles);  // NOLINT

  /**
   * \brief Creates an File Index and returns the index server locations.
   * \param[in] request the index creation request.
   * \param[out] filled by the index server's location.
   * \return Status::OK if success.
   */
  Status create_index(const RpcIndexCreateRequest &request,
                      RpcIndexLocation *index_location);


  /**
   * \brief Locates the index servers as well as the index roots for files.
   * \param[in] lookup_request The request of index lookup.
   * \param[out] results a list of RpcIndexLocation corresponding to every
   * input file path.
   */
  Status locate_index(const RpcIndexLookupRequest &lookup_request,
                      RpcIndexLocationList *results);

 private:
  FRIEND_TEST(MasterControllerTest, TestCreateIndex);

  shared_ptr<TServer> server_;

  unique_ptr<IndexNamespaceInterface> index_namespace_;

  unique_ptr<PartitionManagerInterface> index_partition_manager_;

  unique_ptr<ServerManager> index_server_manager_;

  unique_ptr<ServerManager> master_server_manager_;

  unique_ptr<Namespace> namespace_;

  bool is_config_node_;

  DISALLOW_COPY_AND_ASSIGN(MasterController);
};

}  // namespace masterd
}  // namespace vsfs
#endif  // VSFS_MASTERD_MASTER_CONTROLLER_H_
