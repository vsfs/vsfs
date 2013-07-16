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

#ifndef VSFS_MASTERD_MOCK_INDEX_NAMESPACE_H_
#define VSFS_MASTERD_MOCK_INDEX_NAMESPACE_H_

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include "vsfs/masterd/index_namespace_interface.h"

using std::string;
using std::vector;

namespace vsfs {
namespace masterd {

/**
 * \brief The mock class of IndexNamespace.
 */
class MockIndexNamespace : public IndexNamespaceInterface {
 public:
  MOCK_METHOD0(init, Status());
  MOCK_METHOD2(insert, Status(const string&, const string&));
  MOCK_METHOD2(remove, Status(const string&, const string&));
  MOCK_METHOD1(remove, Status(const string&));
  MOCK_CONST_METHOD3(find, Status(const string&, const string&, string*));
  MOCK_METHOD2(collect, vector<string>(const string&, const string&));
  MOCK_METHOD1(get_index_names, vector<string>(const string&));
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_MOCK_INDEX_NAMESPACE_H_
