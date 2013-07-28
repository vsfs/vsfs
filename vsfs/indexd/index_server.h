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

#ifndef VSFS_INDEXD_INDEX_SERVER_H_
#define VSFS_INDEXD_INDEX_SERVER_H_

#include <string>
#include <vector>
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/vsfs_types.h"

using std::string;
using std::vector;

namespace vsfs {
namespace indexd {

class IndexController;

/**
 * \brief RPC handler for IndexController.
 */
class IndexServer : public IndexServerIf {
 public:
  explicit IndexServer(IndexController *server);

  virtual ~IndexServer();

  virtual void create_index(const RpcIndexCreateRequest &index);

  virtual void remove_index(const RpcIndexName &name);

  virtual void update(const RpcIndexUpdate &updates);

  virtual void search(vector<int64_t>& results,  // NOLINT
                      const RpcComplexQuery& query);

  virtual void info(RpcIndexInfo &info, const RpcIndexInfoRequest &request);

 private:
  IndexController *server_;
};

}  // namespace indexd
}  // namespace vsfs
#endif  // VSFS_INDEXD_INDEX_SERVER_H_
