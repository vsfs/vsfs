/**
 * \brief    POSIX Namespace Translation
 *
 * Copyright 2011 (c) Lei Xu <eddyxu@gmail.com>
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
 * \class PosixPath
 * \brief Represents a posix file path
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
