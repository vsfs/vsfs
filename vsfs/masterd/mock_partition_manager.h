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

#ifndef VSFS_MASTERD_MOCK_PARTITION_MANAGER_H_
#define VSFS_MASTERD_MOCK_PARTITION_MANAGER_H_

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include "vsfs/masterd/partition_manager_interface.h"

using std::string;
using std::vector;

namespace vsfs {
namespace masterd {

class MockPartitionManager : public PartitionManagerInterface {
 public:
  MOCK_METHOD0(init, Status());
  MOCK_METHOD1(add_index, Status(const string&));
  MOCK_METHOD1(remove_index, Status(const string&));
  MOCK_METHOD2(add_partition, Status(const string&, HashValueType));
  MOCK_METHOD2(remove_partition, Status(const string&, HashValueType));
  MOCK_METHOD2(get_partition_path, string(const string&, HashValueType));
  MOCK_METHOD2(get_all_partitions, Status(const string&, vector<string>*));
  MOCK_METHOD2(copy_partition_map, Status(const string&, PartitionMap*));
};

}  // namespace masterd
}  // namespace vsfs

#endif  // VSFS_MASTERD_MOCK_PARTITION_MANAGER_H_
