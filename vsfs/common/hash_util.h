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

#ifndef VSFS_COMMON_HASH_UTIL_H_
#define VSFS_COMMON_HASH_UTIL_H_

#include <cstdint>
#include <string>
#include "vsfs/common/types.h"
#include "vobla/macros.h"

using std::string;

namespace vsfs {

class HashUtil {
 public:
  static FilePathHashType file_path_to_hash(const string &path);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HashUtil);
};

}  // namespace vsfs

#endif  // VSFS_COMMON_HASH_UTIL_H_
