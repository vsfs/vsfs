/*
 * Copyright 2012 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_CLIENT_VSFS_RPC_CLIENT_H_
#define VSFS_CLIENT_VSFS_RPC_CLIENT_H_

#include <transport/TBufferTransports.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include "vobla/thread_pool.h"
#include "vsfs/client/vsfs_client.h"
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/rpc/MetaServer.h"
#include "vsfs/rpc/rpc_client.h"

using std::string;
using std::unique_ptr;
using apache::thrift::transport::TFramedTransport;
using vobla::Status;

namespace vsfs {

class ComplexQuery;

namespace client {

/**
 * \class VSFSRpcClient vsfs_rpc_client.h "vsfs/cllient/vsfs_rpc_client.h"
 * \brief VSFS Rpc Client.
 *
 * This encapures the [Thrift](http://thrift.apache.org/) client for VSFS
 * cluster. Every form of VSFS implementation, such as FUSE or distributed,
 * should use this class to commnunicate with VSFS.
 *
 * \note You should call VSFSClient::CreateVSFSRpcClient(), the factory method,
 * to create this RPC client.
 * \see VSFSClient for the interface defination and usages.
 * \see \b "vsfs/rpc/rpc.thrift" for the thrift IDL definations.
 */
class VSFSRpcClient : public VSFSClient {
 public:
  typedef rpc::RpcClient<MasterServerClient, TFramedTransport> MasterClientType;

  VSFSRpcClient(const string &host, int port);

  /**
   * \brief Constructs a VSFS RPC client with an established master daemon
   * client.
   * \param master a master client instance. This class takes the ownership of
   * the 'master' client instance.
   *
   * It is mainly used for dependency injection.
   */
  // VSFSRpcClient(MasterClientType *master,
  //              IndexServerClientFactory* index_server_client_factory);

  virtual ~VSFSRpcClient();

  Status init();

  /// Connects to the MasterNode with its host name and port number.
  Status connect(const string &host, int port);

  /// Disconnects from the MasterNode.
  Status disconnect();

  Status create(const string &path, mode_t mode);

  Status open(const string &path, int flag);

 private:
  unique_ptr<MasterClientType> master_client_;

  std::mutex master_lock_;

  /// Master node host name.
  string host_;

  /// Master node port.
  int port_;

  vobla::ThreadPool thread_pool_;
};

}  // namespace client
}  // namespace vsfs

#endif  // VSFS_CLIENT_VSFS_RPC_CLIENT_H_