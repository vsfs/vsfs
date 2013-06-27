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

#ifndef VSFS_METAD_META_SERVER_H_
#define VSFS_METAD_META_SERVER_H_

#include <string>
#include <vector>
#include "vobla/macros.h"
#include "vsfs/rpc/MetaServer.h"
#include "vsfs/rpc/vsfs_types.h"

using std::string;
using std::vector;

namespace vsfs {
namespace metad {

class MetaController;

/**
 * \brief RPC server for MetaServer.
 */
class MetaServer : public MetaServerIf {
 public:
  explicit MetaServer(MetaController *ctrl);

  virtual ~MetaServer();

  virtual void insert(const RpcMetaDataList& metadata);

  virtual void remove(int64_t file_id);

  virtual void find(string& result, int64_t file_id);  // NOLINT

  virtual void search(vector<string>& file_paths,  // NOLINT
                      const vector<int64_t>& file_ids);

 private:
  MetaController* controller_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(MetaServer);
};

}  // namespace metad
}  // namespace vsfs

#endif  // VSFS_METAD_META_SERVER_H_
