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

#include <string.h>
#include <memory>
#include <string>
#include "vobla/hash.h"
#include "vsfs/common/hash_util.h"

using std::string;
using vobla::MD5Digest;

namespace vsfs {

HashUtil::FilePathHashType HashUtil::file_path_to_hash(const string &path) {
  FilePathHashType ret;
  // TODO(lxu): add a flag to switch hash function.
  MD5Digest hash_value(path);
  memcpy(&ret, hash_value.digest(), sizeof(ret));
  return ret;
}
}  // namespace vsfs