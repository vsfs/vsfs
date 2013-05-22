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

#ifndef VSFS_COMMON_MOCK_KEY_VALUE_STORE_H_
#define VSFS_COMMON_MOCK_KEY_VALUE_STORE_H_

#include <gmock/gmock.h>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/key_value_store.h"

using std::string;
using vobla::Status;

namespace vsfs {

class MockKeyValueStore : public KeyValueStore {
 public:
  MOCK_METHOD0(open, Status());
  MOCK_METHOD2(get, Status(const string&, string*));
  MOCK_METHOD2(put, Status(const string&, const string&));
  MOCK_METHOD1(remove, Status(const string&));
};

}  // namespace vsfs

#endif  // VSFS_COMMON_MOCK_KEY_VALUE_STORE_H_
