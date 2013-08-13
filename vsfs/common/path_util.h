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

#ifndef VSFS_COMMON_PATH_UTIL_H_
#define VSFS_COMMON_PATH_UTIL_H_

#include <string>
#include "vsfs/common/types.h"

using std::string;

namespace vsfs {

class PathUtil {
 public:
  /// Calculates the hash value (MD5) of the path.
  static HashValueType path_to_hash(const string& path);

  /// Generates the index path for the index on 'root' with index 'name'.
  static string index_path(const string& root, const string& name);

  /// Splits an index path into two parts (root, name).
  static bool split_index_path(const string& path, string* root, string* name);

  /// Generates the partition path with index 'root', 'name' and partition
  /// hash range starting point.
  static string partition_path(const string& root, const string& name,
                               HashValueType sep);
 private:
  PathUtil() = delete;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_PATH_UTIL_H_
