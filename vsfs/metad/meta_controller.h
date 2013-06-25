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

#ifndef VSFS_METAD_META_CONTROLLER_H_
#define VSFS_METAD_META_CONTROLLER_H_

#include <boost/shared_ptr.hpp>
#include <server/TThreadedServer.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "vobla/status.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/metad/meta_manager.h"

using apache::thrift::server::TThreadedServer;
using boost::shared_ptr;
using std::condition_variable;
using std::mutex;
using std::thread;
using std::string;
using std::unique_ptr;
using std::vector;
using vsfs::MasterServerClient;
using vsfs::rpc::RpcClient;

namespace vsfs {
namespace metad {

class MetaController {
 public:
  typedef RpcClient<MasterServerClient> MasterClientType;

  MetaController();

  virtual ~MetaController();

  /// Start this server.
  void start();

  /// Stop this server.
  void stop();

  /// Join this Meta Server to the CH ring in master.
  Status join();

  /**
   * \brief Insert a record into key-value storage.
   * \param[in] file_id The unique id of the file.
   * \param[in] file_path The path of the file.
   */
  virtual Status insert(uint64_t file_id, const string &file_path);

  /**
   * \brief Insert a bunch of <file_id, file_path> pair.
   */
  virtual Status insert(const RpcMetaDataList& metadata);

  /**
   * \brief Remove a record from the key-value storage.
   * \param[in] file_id The unique id of the file.
   */
  virtual Status remove(uint64_t file_id);

  /**
   * \brief Given a file_id, return the corresponding file_path.
   * \param[in] file_id The unique id of the file.
   * \param[out] The path of the file.
   */
  virtual Status find(uint64_t file_id, string *file_path);

  virtual Status find(const vector<int64_t>& file_ids,
                      vector<string>* results);

  MasterServerClient* master_connection() const {
    return master_->handler();
  }

 private:
  string host_;

  /// Listen port.
  int port_;

  /// Thrift RPC server.
  shared_ptr<TThreadedServer> server_;

  /// Meta Manager instance.
  unique_ptr<MetaManager> manager_;

  /// Connection to master
  unique_ptr<MasterClientType> master_;

  void background_task();

  thread background_thread_;

  condition_variable background_cv_;

  mutex back_ground_mutex_;
};

}  // namespace metad
}  // namespace vsfs

#endif  // VSFS_METAD_META_CONTROLLER_H_
