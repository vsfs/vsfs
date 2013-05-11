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
 * \file vsfs/common/posix_path.h'
 * \brief POSIX Namespace Translation
 */

#ifndef VSFS_COMMON_POSIX_PATH_H_
#define VSFS_COMMON_POSIX_PATH_H_

#include <string>

using std::string;

namespace vobla {
class Status;
}

namespace vsfs {

class ComplexQuery;

/**
 * \class PosixPath vsfs/common/posix.h
 * \brief Represents a posix file path.
 */
class PosixPath {
 public:
  /// Constructs an empty PosixPath.
  PosixPath() = default;

  ~PosixPath() = default;

  explicit PosixPath(const string &path);

  /// Returns true if this path represents a query.
  bool is_query() const;

  /// Returns true if the path represents a query result.
  bool is_result() const;

  /**
   * \brief Returns the root that the query starts.
   *
   * For example, for a query of '/home/eddyxu/data/set1/?energy>10/',
   * its root is '/home/eddyxu/data/set1'
   */
  const string& root() const {
    return root_;
  }

  /// Returns the path.
  const string& path() const {
    return path_;
  }

  /// Returns the query string.
  const string& query() const {
    return query_;
  }

  /// Returns the result string.
  const string& result() const {
    return result_;
  }

  /**
   * \brief Generates the complex query from this directory.
   * \return Status::OK if this posix path can be successfully translated into
   * a ComplexQuery. Otherwise a failure status will be returned.
   */
  vobla::Status complex_query(ComplexQuery *query);

 private:
  string path_;
  string root_;
  string query_;
  string result_;
};

}  // namespace vsfs
#endif  // VSFS_COMMON_POSIX_PATH_H_
