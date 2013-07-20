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

#include <string>
#include "vsfs/common/test_leveldb_store.h"

namespace vsfs {

Status TestLevelDBStore::get(const string& key, string* value) {
  auto it = store_.find(key);
  if (it == store_.end()) {
    return Status(-ENOENT, "The key does not exist.");
  }
  *value = it->second;
  return Status::OK;
}

Status TestLevelDBStore::put(const string& key, const string &value) {
  store_[key] = value;  // It might overwrite the old value.
  return Status::OK;
}

Status TestLevelDBStore::remove(const string& key) {
  store_.erase(key);
  return Status::OK;
}

}  // namespace vsfs
