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

#include <glog/logging.h>
#include <algorithm>
#include <string>
#include "vobla/traits.h"
#include "vsfs/index/index_info.h"

namespace vsfs {
namespace index {

int IndexInfo::string_to_index_type(const string &str) {
  string buf = str;
  std::transform(buf.begin(), buf.end(), buf.begin(), ::tolower);
  if (buf == "btree") {
    return BTREE;
  } else if (buf == "hash") {
    return HASH;
  } else if (buf == "content") {
    return CONTENT;
  } else if (buf == "multidim") {
    return MULTIDIM;
  }
  return -1;
}

string IndexInfo::index_type_to_string(int idx_type) {
  switch (idx_type) {
    case BTREE:
      return "btree";
    case HASH:
      return "hash";
    case CONTENT:
      return "content";
    case MULTIDIM:
      return "multidim";
    default:
      return "unknown";
  }
}

IndexInfo::IndexInfo() : index_type_(UNKNOWN_INDEX), key_type_(UNKNOWN) {
}

IndexInfo::IndexInfo(const string &idx_path, const string &name,
                     int idx_type, int k_type )
    : index_path_(idx_path), index_name_(name), index_type_(idx_type),
    key_type_(k_type) {
}

bool IndexInfo::is_validate() const {
  if (index_type_ == BTREE || index_type_ == MULTIDIM) {
    return key_type_ == INT64 || key_type_ == UINT64 ||
        key_type_ == FLOAT || key_type_ == DOUBLE;
  }
  if (index_type_ == CONTENT) {
    return key_type_ == STRING;
  }
  return true;
}

void IndexInfo::set_path(const string &index_path) {
  index_path_ = index_path;
}

const string& IndexInfo::path() const {
  return index_path_;
}

void IndexInfo::set_index_name(const string &name) {
  CHECK(!name.empty());
  index_name_ = name;
}

const string& IndexInfo::index_name() const {
  return index_name_;
}

void IndexInfo::set_index_type(int type) {
  CHECK_GE(type, HASH);
  CHECK_LE(type, CONTENT);
  index_type_ = type;
}

int IndexInfo::index_type() const {
  return index_type_;
}

void IndexInfo::set_key_type(int type) {
  CHECK_GT(type, UNKNOWN);
  CHECK_LE(type, STRING);
  key_type_ = type;
}

int IndexInfo::key_type() const {
  return key_type_;
}

}  // namespace index
}  // namespace vsfs
