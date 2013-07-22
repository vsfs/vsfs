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
// #include "vsfs/client/vsfs_rpc_client.h"
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

  /**
   * \brief Main index migration process.
   */
  // virtual Status migrate_index(const index::IndexInfo &idx_info);

  /**
   * \brief migrate a single index.
   */
  // virtual Status migrate(const RpcIndexMigrationData &data);

  /**
   * \brief forward the updates to taker node's log.
   */
  // virtual Status update_to_remote_log(const RpcIndexUpdate& updates);

  /**
   * \brief Ask taker node to join the server ring.
   * This function should be called when there is a new server needs
   * to join the cluster.
   */
  // virtual Status join_taker_node_server();

  /**
   * \brief Ask taker node to join the new index partition to master.
   * This function should be called during index partition process.
   */
  /* virtual Status join_taker_node_index_partition(const RpcIndexInfo& idx_info,
                                                 int64_t sep); */

  /// Returns the connected replication machines.
  // size_t num_replica_machines() const;

  // string get_new_partition_name(const string &old_name, int64_t split_pos);

 private:
  FRIEND_TEST(IndexControllerTest, TestJoinRing);

  /// Status code.
  enum {
    PREPARE, RUNNING, STOPPED,
  };

  /// Running status.
  int status_;

  // void background_migration();

  void background_task();

  /// A background thread to do heard-beating as well as other maintainging
  /// tasks.
  thread backgroup_thread_;

  condition_variable background_cv_;

  mutex background_mutex_;

  string host_;

  int port_;

  string base_dir_;

  // bool forward_data_;

  unique_ptr<IndexManager> manager_;

  /// A connection to the master.
  unique_ptr<MasterClientType> master_;

  /// Thrift RPC server.
  shared_ptr<TServer> server_;

  // typedef RpcClient<IndexServerClient> IndexServerClientType;

  // vector<unique_ptr<IndexServerClientType>> replica_machines_;

  typedef std::pair<int64_t, int64_t> FileIdRangeType;

  // Status deserialize_data_into_index(const RpcIndexMigrationData &data);

  // Status filter_and_forward_migration_updates(const RpcIndexUpdate &updates);

  /**
   * \brief Index Migration Management Table, support multiple index to
   * migrate at the same time. (Each index can only have one migration).
   */
  /* struct RangeIndexMigrationRecord {
    FileIdRangeType range;
    NodeInfo taker_node;
    unique_ptr<IndexControllerClientType> connection;
  };

  unordered_map<string, unique_ptr<RangeIndexMigrationRecord>> migration_map_;

  void add_migration_record(const string &root_path, const string &name,
                            int64_t split_pos, int64_t end_pos,
                            const NodeInfo &node);

  void remove_migration_record(const string &root_path, const string &name);

  RangeIndexMigrationRecord* get_migration_record(
      const string &root_path, const string &name);

  IndexControllerClientType* get_migration_connection(const string &root_path,
                                                  const string &name);

  string get_index_id(const string &root_path, const string &name);

  bool is_in_range(const string &key_str, const FileIdRangeType &range);

  /// manually force merging an index's log.
  Status merge_log_to_index(const string &root_path, const string &name);

  bool forward_data_or_not(); */

  DISALLOW_IMPLICIT_CONSTRUCTORS(IndexController);
};

}  // namespace indexd
}  // namespace vsfs

#endif  // VSFS_INDEXD_INDEX_CONTROLLER_H_
