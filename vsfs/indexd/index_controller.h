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

#ifndef VSFS_INDEXD_INDEX_CONTROLLER_H_
#define VSFS_INDEXD_INDEX_CONTROLLER_H_

#include <boost/shared_ptr.hpp>
#include <gtest/gtest_prod.h>
#include <server/TServer.h>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <unordered_map>
#include <vector>
#include "vobla/macros.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/indexd/index_controller_interface.h"
#include "vsfs/indexd/index_manager.h"

using apache::thrift::server::TServer;
using boost::shared_ptr;
using std::condition_variable;
using std::unordered_map;
using std::mutex;
using std::string;
using std::thread;
using std::unique_ptr;
using std::vector;
using vsfs::MasterServerClient;
using vsfs::rpc::RpcClient;

namespace vsfs {

/**
 * \brief The major component of the IndexController.
 *
 * It manages an in-ram index services for all clients.
 */
namespace indexd {

class IndexManager;

/**
 * \brief The in-ram implementation of IndexController.
 */
class IndexController : public IndexControllerInterface {
 public:
  typedef RpcClient<MasterServerClient> MasterClientType;

  explicit IndexController(const string &basedir);

  IndexController(const string &basedir, const string &host, int port);

  /// Dependency injection.
  IndexController(const string &basedir, const string &host, int port,
                  MasterClientType *master);

  virtual ~IndexController();

  /// Starts the thrift server.
  void start();

  /// Stops the thrift server.
  void stop();

  /// Sends a request to the master to join the consistent hash ring and
  /// sets up connections to the replica IndexController(s).
  Status join();

  /// Processes client's create index operation.
  virtual Status create_index(const string &root_path, const string &name,
                              int index_type, int key_type);

  /**
   * \brief Updates records.
   *
   * TODO(lxu): It might not be wise to use RpcIndexUpdate here. However, if
   * transform it to something like IndexUpdate structure, it will involve a
   * large numbers of memory copy.
   */
  virtual Status update(const RpcIndexUpdate &updates);

  /*
   * \brief Search records.
   */
  virtual Status search(const RpcComplexQuery &query,
                        vector<int64_t>* results);

  /**
   * \brief Gets the information of the particular index.
   */
  virtual Status info(const RpcIndexInfoRequest &request,
                      RpcIndexInfo *result);

  MasterServerClient* master_connection() const {
    return master_->handler();
  }

 private:
  FRIEND_TEST(IndexControllerTest, TestJoinRing);

  /// Status code.
  enum {
    PREPARE, RUNNING, STOPPED,
  };

  /// Running status.
  int status_;

  void background_task();

  /// A background thread to do heard-beating as well as other maintainging
  /// tasks.
  thread backgroup_thread_;

  condition_variable background_cv_;

  mutex background_mutex_;

  string host_;

  int port_;

  string base_dir_;

  unique_ptr<IndexManager> manager_;

  /// A connection to the master.
  unique_ptr<MasterClientType> master_;

  /// Thrift RPC server.
  shared_ptr<TServer> server_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(IndexController);
};

}  // namespace indexd
}  // namespace vsfs

#endif  // VSFS_INDEXD_INDEX_CONTROLLER_H_
