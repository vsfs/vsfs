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

#ifndef VSFS_CLIENT_MOCK_VSFS_CLIENT_H_
#define VSFS_CLIENT_MOCK_VSFS_CLIENT_H_

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include "vsfs/client/vsfs_client.h"
#include "vsfs/common/types.h"

using std::string;
using std::vector;

namespace vsfs {
namespace client {

class MockVSFSClient : public VSFSClient {
 public:
  MOCK_METHOD0(init, Status());
  MOCK_METHOD4(mkdir, Status(const string&, int64_t, int64_t, int64_t));
  MOCK_METHOD1(rmdir, Status(const string&));
  MOCK_METHOD2(readdir, Status(const string&, vector<string>*));
  MOCK_METHOD5(create, Status(const string&, int64_t, int64_t, int64_t,
                              ObjectId*));
  MOCK_METHOD2(open, Status(const string&, ObjectId*));
  MOCK_METHOD2(unlink, Status(const string&, ObjectId*));
  MOCK_METHOD7(create_index, Status(const string&, const string&, int,
                                    int, int64_t, int64_t, int64_t));
  MOCK_METHOD2(remove_index, Status(const string&, const string&));
  MOCK_METHOD2(search, Status(const ComplexQuery&, vector<string>*));
  MOCK_METHOD1(update, Status(const vector<IndexUpdateRequest>&));
  MOCK_METHOD3(get_index_infos, Status(const string&,
                                       vector<index::IndexInfo>*,
                                       bool recursive));
};

}  // namespace client
}  // namespace vsfs

#endif  // VSFS_CLIENT_MOCK_VSFS_CLIENT_H_
