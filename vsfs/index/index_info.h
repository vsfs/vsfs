/*
 * Copyright 2012-2013 (c) Lei Xu <eddyxu@gmail.com>
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

/**
 * \file vsfs/index/index_info.h
 * \brief The information/configurations of an file index.
 */

#ifndef VSFS_INDEX_INDEX_INFO_H_
#define VSFS_INDEX_INDEX_INFO_H_

#include <string>

using std::string;

namespace vsfs {
namespace index {

/**
 * \class IndexInfo index.h "vsfs/index/index_info.h"
 * \brief The description of a file index.
 */
class IndexInfo {
 public:
  /// Index Type.
  enum {
    UNKNOWN_INDEX, HASH, BTREE, MULTIDIM, CONTENT
  };

  static int string_to_index_type(const string &str);

  static string index_type_to_string(int idx_type);

  IndexInfo();

  IndexInfo(const string &index_path, const string &name,
            int index_type, int key_type);

  bool is_validate() const;

  void set_path(const string &index_path);

  const string& path() const;

  void set_index_name(const string &name);

  const string& index_name() const;

  void set_index_type(int type);

  int index_type() const;

  /**
   * \brief Returns index type in string format.
   */
  string index_type_string() const;

  void set_key_type(int type);

  int key_type() const;

  /// Returns key type in string format.
  string key_type_string() const;

 private:
  string index_path_;

  string index_name_;

  int index_type_;

  int key_type_;
};

}  // namespace index
}  // namespace vsfs

#endif  // VSFS_INDEX_INDEX_INFO_H_
