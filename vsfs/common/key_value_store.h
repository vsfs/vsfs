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

#ifndef VSFS_COMMON_KEY_VALUE_STORE_H_
#define VSFS_COMMON_KEY_VALUE_STORE_H_

#include <string>
#include "vobla/status.h"

using std::string;
using vobla::Status;

namespace vsfs {

/**
 * \class KeyValueStore "vsfs/common/key_value_store.h"
 * \brief An abstract class to define the interface of any Key-Value type
 * of storage.
 *
 * You can access the data through the typical K-V DB operations:
 *  - put(key, value)
 *  - get(key, &value)
 *  - remove(key)
 */
class KeyValueStore {
 public:
  /**
   * \brief Opens an KeyValueStore. The default behavior of `open()` is
   * creating the key value store if it does not existed.
   */
  virtual Status open() = 0;

  virtual Status get(const string& key, string* value) = 0;

  virtual Status put(const string& key, const string &value) = 0;

  virtual Status remove(const string& key) = 0;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_KEY_VALUE_STORE_H_
