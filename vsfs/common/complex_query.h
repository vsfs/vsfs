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
 * \file vsfs/common/complex_query.h
 * \brief Complex file query.
 */

#ifndef VSFS_COMMON_COMPLEX_QUERY_H_
#define VSFS_COMMON_COMPLEX_QUERY_H_

#include <boost/lexical_cast.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "vobla/range.h"

using boost::lexical_cast;
using std::string;
using std::unordered_map;
using std::vector;

namespace vobla {
class Clock;
class Status;
}

/**
 * \namespace vsfs
 * \brief Versatile Searchable File System.
 */
namespace vsfs {

/**
 * \brief Represents range queries in string.
 * \TODO(eddyxu): move to internal space?
 */
class StringRange {
 public:
  StringRange() = default;

  /// Constructs a StringRange from Range class.
  template<typename Range>
  explicit StringRange(const Range& range) {
    lower = lexical_cast<string>(range.lower());
    lower_closed = range.is_lower_closed();
    upper = lexical_cast<string>(range.upper());
    upper_closed = range.is_upper_closed();
  }

  ~StringRange() = default;

  string lower;
  bool lower_closed;
  string upper;
  bool upper_closed;
};

/**
 * \class ComplexQuery
 * \brief Represents a complex file query.
 */
class ComplexQuery {
 public:
  /**
   * \brief Sets the internal clock used to calculate the absolute time from
   * the related time from query.
   *
   * \note This function should only be used for testing purpose.
   */
  static void set_clock(vobla::Clock *clock);

  /// Default constructor
  ComplexQuery();

  ~ComplexQuery() = default;

  /**
   * \brief Parses a query path and fills the fields of complex query.
   * \param query_path a absolute path including the query.
   */
  vobla::Status parse(const string &query_path);

  /**
   * \brief Returns the root directory of this query.
   *
   * All returned files are under this root directory.
   */
  const string& root() const { return root_; }

  void set_root(const string& new_root) { root_ = new_root; }

  /**
   * \brief Gets the index of all range queries.
   */
  void get_names_of_range_queries(vector<string> *names) const;

  StringRange* range_query(const string &name);

  string debug_string() const;

 private:
  vobla::Status parse_segment(const string& seg);
  vobla::Status parse_condition(const string& buf);
  vobla::Status parse_expression(const string& exp);

  /// The top directoy this query starts.
  string root_;

  typedef unordered_map<string, StringRange> RangeQueryMap;
  RangeQueryMap range_query_map_;
};

}  // namespace vsfs

#endif  // VSFS_COMMON_COMPLEX_QUERY_H_
