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

class IndexNamespace;
class IndexServerManager;
class PartitionManager;

/**
 * \brief MasterController the main control block of master server.
 */
class MasterController {
 public:
  MasterController();

  virtual ~MasterController();

  /**
   * \brief Starts the Masterd RPC server.
   */
  void start();

  /**
   * \brief Stops the masterd RPC server.
   */
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

 private:
  shared_ptr<TServer> server_;

  unique_ptr<IndexNamespace> index_namespace_;

  unique_ptr<PartitionManager> index_partition_manager_;

  unique_ptr<IndexServerManager> index_server_manager_;

  DISALLOW_COPY_AND_ASSIGN(MasterController);
};

}  // namespace masterd
}  // namespace vsfs
#endif  // VSFS_MASTERD_MASTER_CONTROLLER_H_
