/**
 * \file master_server.h
 *
 * \brief The central data structure of Master node.
 *
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
 */

#ifndef VSFS_MASTERD_MASTER_CONTROLLER_H_
#define VSFS_MASTERD_MASTER_CONTROLLER_H_

#include <boost/shared_ptr.hpp>
#include <server/TServer.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "vobla/macros.h"
#include "vobla/status.h"
#include "vsfs/rpc/vsfs_types.h"

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

  void start();

  void stop();

  /**
   * \brief Lets an IndexServer join the consistent hash ring, and
   * send the address of its replica machines back to it.
   *
   * \param node The node information of the joining index server.
   * \param replicas It will be filled with the addresses for the replica
   * servers.
   */
  Status join_index_server(const NodeInfo &node,
                           RpcNodeAddressList *replicas);

  /**
   * \brief Inserts an IndexServer to a particular position.
   */
  Status join_index_server(uint64_t pos, const NodeInfo &node,
                           RpcNodeAddressList *replicas);

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
