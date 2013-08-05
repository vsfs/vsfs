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

#include <boost/shared_ptr.hpp>
#include <transport/TBufferTransports.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>  // NOLINT
#include "vobla/thread_pool.h"
#include "vsfs/client/vsfs_client.h"
#include "vsfs/common/types.h"
#include "vsfs/common/server_map.h"
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/rpc_client_factory.h"

using std::string;
using std::unique_ptr;
using apache::thrift::transport::TFramedTransport;
using vobla::Status;

namespace vsfs {

class ComplexQuery;

using rpc::RpcClientFactoryInterface;

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
  typedef rpc::RpcClient<IndexServerClient, TFramedTransport> IndexClientType;
  typedef RpcClientFactoryInterface<MasterClientType> MasterClientFactory;
  typedef RpcClientFactoryInterface<IndexClientType> IndexClientFactory;

  VSFSRpcClient(const string& host, int port);

  /**
   * \brief Constructs a VSFS RPC client with an established master daemon
   * client.
   * \param master a master client instance. This class takes the ownership of
   * the 'master' client instance.
   *
   * It is mainly used for dependency injection.
   */
  VSFSRpcClient(MasterClientFactory* master_factory,
                IndexClientFactory* index_factory);

  virtual ~VSFSRpcClient();

  Status init();

  /**
   * \brief connects to the primary MasterNode and obtains the master node
   * map.
   */
  Status connect(const string& host, int port);

  /// Disconnects from the MasterNode.
  Status disconnect();

  Status create(const string& path, int64_t mode, int64_t uid, int64_t gid,
                ObjectId* id);

  Status open(const string& path, ObjectId *id);

  Status unlink(const string& path);

  /**
   * \brief Creates a new directory on path.
   * \param path the absolute path of directory to be created.
   * \param mode the mode of the new created directory.
   * \param uid the user id of the new created directory.
   * \param gid the group id of the new created directory.
   * \return Status::OK if success.
   */
  Status mkdir(const string& path, int64_t mode, int64_t uid, int64_t gid);

  /// Deletes a directory.
  Status rmdir(const string& path);

  /// Reads all subfiles from a directory.
  Status readdir(const string& dirpath, vector<string>* files);  // NOLINT

  Status create_index(const string& index_path,
                      const string& index_name,
                      int index_type,
                      int key_type);

  Status search(const ComplexQuery& query,
                vector<string>* results);

  Status update(const vector<IndexUpdateRequest>& updates);

  Status import(const vector<string>& file_paths);

  Status info(const string& path, vector<index::IndexInfo>* infos);

 private:
  /// Returns true if this client has been initialized.
  bool is_initialized();

  Status add_subfile(const string& filepath);

  unique_ptr<MasterClientFactory> master_client_factory_;

  unique_ptr<IndexClientFactory> index_client_factory_;

  boost::shared_ptr<MasterClientType> master_client_;  // primary master

  ServerMap master_map_;

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
