// vim:ts=2:sw=2:sts=2:et:ft=thrift
/*
 * Copyright 2011-2013 (c) Lei Xu <eddyxu@gmail.com>
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

namespace cpp vsfs.masterd
namespace py vsfs.masterd

/**
 * The metadata entry stored in the namespace.
 */
struct FileMetadata {
  1: i64 object_id,
  2: i32 mode,
  3: i32 gid,
  4: i32 uid,
  5: i64 size,
  6: double atime,
  7: double ctime,
  8: double mtime,
}

struct IndexNamespaceNode {
  1: string path,
  2: set<string> names,
}

struct IndexPartition {
  1: i64 hash_sep,  // Hash Separator
  2: string path,
}

/**
 * \brief A Consistent Hash Ring for one file index.
 */
struct IndexConsistentHashRing {
  1: list<IndexPartition> partitions,
}
