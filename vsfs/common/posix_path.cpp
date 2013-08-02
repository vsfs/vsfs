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

/**
 * \brief Wraps operations on posix path.
 */

#include <glog/logging.h>
#include <string>
#include "vobla/status.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/common/posix_path.h"

using vobla::Status;

namespace vsfs {

PosixPath::PosixPath(const string& posix_path) : path_(posix_path) {
  CHECK(!posix_path.empty());
  size_t question_mark = posix_path.find('?');
  if (question_mark == string::npos) {
    root_ = posix_path;
    return;
  }
  size_t result_begin = posix_path.find('/', question_mark);
  root_ = posix_path.substr(0, question_mark - 1);
  if (result_begin == string::npos) {
    query_ = posix_path.substr(question_mark + 1);
  } else {
    query_ = posix_path.substr(question_mark + 1,
                               result_begin - question_mark - 1);
    result_ = posix_path.substr(result_begin + 1);
  }
}

bool PosixPath::is_validate() const {
  // TODO(eddyxu): check the valid of path.
  return true;
}

bool PosixPath::is_query() const {
  return !query_.empty() && result_.empty();
}

bool PosixPath::is_result() const {
  return !query_.empty() && !result_.empty();
}

Status PosixPath::complex_query(ComplexQuery *result) {
  CHECK_NOTNULL(result);
  return result->parse(path_);
}

}  // namespace vsfs
