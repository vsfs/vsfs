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

#include <boost/filesystem.hpp>
#include <string.h>
#include <string>
#include "vobla/hash.h"
#include "vsfs/common/path_util.h"

using std::string;
using std::to_string;
using vobla::MD5Digest;
namespace fs = boost::filesystem;

namespace vsfs {

HashValueType PathUtil::path_to_hash(const string& path) {
  HashValueType ret;
  // TODO(lxu): add a flag to switch hash function.
  MD5Digest hash_value(path);
  memcpy(&ret, hash_value.digest(), sizeof(ret));
  return ret;
}

string PathUtil::index_path(const string& root, const string& name) {
  return (fs::path(root) / ".vsfs" / name).string();
}

string PathUtil::partition_path(const string& root, const string& name,
                                HashValueType sep) {
  return (fs::path(root) / ".vsfs" / name / to_string(sep)).string();
}

}  // namespace vsfs
