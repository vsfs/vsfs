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

#ifndef VSFS_MASTERD_MASTER_SERVER_H_
#define VSFS_MASTERD_MASTER_SERVER_H_

#include <string>
#include <vector>
#include "vobla/macros.h"
#include "vsfs/rpc/MasterServer.h"

using std::string;
using std::vector;

namespace vsfs {
namespace masterd {

class MasterController;

/**
 * \brief The RPC server for Master Server.
 */
class MasterServer : public vsfs::MasterServerIf {
 public:
  /**
   * \brief Constructs a MasterServer with its parent controller.
   * \note This server does not have the ownership of the MasterController
   */
  explicit MasterServer(MasterController* controller);

  ~MasterServer();

  void join_index_server(RpcNodeAddressList& _return,  // NOLINT
                         const RpcNodeInfo& info);

  /**
   * \brief Creates index in the master server and assign a IndexServer to it.
   */
  void create_index(RpcIndexLocation&, const RpcIndexCreateRequest&);

 private:
  MasterController* controller_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(MasterServer);
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_MASTER_SERVER_H_
