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
 * \file vsfs/common/complex_query.cpp
 * \brief VSFS complex file query.
 */

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <string>
#include <vector>
#include "vobla/clock.h"
#include "vobla/map_util.h"
#include "vobla/status.h"
#include "vsfs/common/complex_query.h"

using boost::lexical_cast;
using boost::algorithm::trim_copy;
using std::string;
using std::stringstream;
using std::to_string;
using std::vector;
using vobla::Clock;
using vobla::Status;
using vobla::append_keys_from_map;
using vobla::find_or_null;

namespace vsfs {

static Clock* global_clock = Clock::real_clock();

void ComplexQuery::set_clock(Clock *clock) {
  global_clock = clock;
}

ComplexQuery::ComplexQuery() {
}

namespace {

enum {
  EQ,  /*!< equal */
  NEQ, /*!< not equal */
  GT,  /*!< greater than */
  GE,  /*!< greater or equal */
  LT,  /*!< less than */
  LE   /*!< less or equal */
};

/// Parses the conditional symbol.
int parse_cond_op(const string& op) {
  if (op == "=") {
    return EQ;
  } else if (op == "!=") {
    return NEQ;
  } else if (op == ">") {
    return GT;
  } else if (op == ">=") {
    return GE;
  } else if (op == "<") {
    return LT;
  } else if (op == "<=") {
    return LE;
  } else {
    return -1;
  }
}

/// Parses the numeric value.
Status parse_value(const string& field, const string &value,
                   string *new_value) {
  static const uint64_t DAY = 60 * 60 * 24;
  static const uint64_t WEEK = DAY * 7;
  static const uint64_t KB = 1024;
  static const uint64_t MB = KB * 1024;
  static const uint64_t GB = MB * 1024;
  static const uint64_t TB = GB * 1024;

  string num;
  uint64_t num_value = 1;
  size_t unit_pos = value.find_first_not_of("0123456789.+-");
  if (unit_pos != string::npos) {
    string unit = value.substr(unit_pos);
    if (field == "atime" || field == "ctime" || field == "mtime") {
      if (unit == "d") {
        num_value = DAY;
      } else if (unit == "w") {
        num_value = WEEK;
      } else {
        return Status(-1, string("Un-recognized time unit: ") + unit);
      }
    } else if (field == "size") {
      if (unit == "k") {
        num_value = KB;
      } else if (unit == "m") {
        num_value = MB;
      } else if (unit == "g") {
        num_value = GB;
      } else if (unit == "t") {
        num_value = TB;
      } else {  // other field should not support unit
        return Status(-1, string("Un-recognized file-size unit: ") + unit);
      }
    }
    num = value.substr(0, unit_pos);
  } else {
    *new_value = value;
    return Status::OK;
  }

  if (field == "atime" || field == "ctime" || field == "mtime") {
    bool has_symbol = (value[0] == '+' || value[0] == '-');
    if (has_symbol) {
      int symbol = (value[0] == '+') ? 1 : -1;
      uint64_t now = static_cast<uint64_t>(global_clock->now());
      *new_value = to_string(
          now + symbol * (num_value * lexical_cast<uint64_t>(num.substr(1))));
      return Status::OK;
    }
  }
  *new_value = to_string(num_value * lexical_cast<uint64_t>(num));
  return Status::OK;
}

}  // namespace

Status ComplexQuery::parse_expression(const string& exp) {
  static const string OPCHAR = "=!<>";
  CHECK(!exp.empty());

  size_t op_pos = exp.find_first_of(OPCHAR);
  if (op_pos == string::npos) {
    return Status(-1, "Bad expression.");
  }
  size_t value_pos = exp.find_first_not_of(OPCHAR, op_pos);
  if (value_pos == string::npos) {
    return Status(-1, "Bad expression.");
  }
  string field = trim_copy(exp.substr(0, op_pos));
  string opstr = exp.substr(op_pos, value_pos - op_pos);
  string value = trim_copy(exp.substr(value_pos));

  int op = parse_cond_op(opstr);
  if (op < 0) {
    return Status(-1, "Bad operation.");
  }

  string tmp(value);
  Status status = parse_value(field, tmp, &value);
  if (!status.ok()) {
    LOG(ERROR) << status.message() << ":" << tmp;
    return status;
  }

  switch (op) {
    case LE:
      range_query_map_[field].upper = value;
      range_query_map_[field].upper_closed = true;
      break;
    case LT:
      range_query_map_[field].upper = value;
      range_query_map_[field].upper_closed = false;
      break;
    case GE:
      range_query_map_[field].lower = value;
      range_query_map_[field].lower_closed = true;
      break;
    case GT:
      range_query_map_[field].lower = value;
      range_query_map_[field].lower_closed = false;
      break;
    case EQ:
      range_query_map_[field].upper = value;
      range_query_map_[field].upper_closed = true;
      range_query_map_[field].lower = value;
      range_query_map_[field].lower_closed = true;
      break;
    default:
      LOG(ERROR) << "Unknown op code: " << opstr;
      return Status(-1, "Unknown op code.");
  }
  return Status::OK;
}

Status ComplexQuery::parse_condition(const string& buf) {
  CHECK(!buf.empty());
  Status status;
  size_t pos = 0;
  while (pos != string::npos) {
    size_t last = pos;
    pos = buf.find('&', last);
    string exp;
    if (pos == string::npos) {
      exp = buf.substr(last);
    } else {
      exp = buf.substr(last, pos - last);
    }
    status = parse_expression(exp);
    if (!status.ok()) {
      return status;
    }
    if (pos != string::npos) {
      pos++;
    }
  }
  return status;
}

Status ComplexQuery::parse_segment(const string& seg) {
  VLOG(1) << "Parse Seg: " << seg;
  if (seg.empty()) {
    return Status(-1, "Segment is empty.");
  }
  Status status;
  if (seg.find_first_of("<>=!") != string::npos) {  // conditional expression
    status = parse_condition(seg);
  }
  return status;
}

Status ComplexQuery::parse(const string &buf) {
  VLOG(1) << "Parse path into ComplexQuery: " << buf;
  CHECK(!buf.empty());
  CHECK_EQ(buf[0], '/');  // checks buf is an absolute path.

  Status status;
  size_t pos = 0;

  // The query expression must starts with a question mark '?'.
  pos = buf.find('?');
  if (pos == string::npos) {
    return Status(-2, string("This path is not a file query.") + buf);
  }

  if (pos > 1) {
    root_ = buf.substr(0, pos-1);
  } else {
    root_ = "/";
  }
  VLOG(1) << "Start point is: " << root_;
  pos++;

  while (pos != string::npos) {
    size_t last = pos;
    pos = buf.find("/", last + 1);
    if (pos == last + 1) {  // for the case of "&&" or "//"
      continue;
    }

    string segment;
    if (pos == string::npos) {
      segment = buf.substr(last);
    } else {
      segment = buf.substr(last + 1, pos - last - 1);
      pos++;
    }
    VLOG(0) << "ComplexQuery::parse: " << segment;
    status = parse_segment(segment);
    if (!status.ok()) {
      return status;
    }
  }
  return Status::OK;
}

string ComplexQuery::debug_string() const {
  string ret("ComplexQuery(");
  ret += root_ + "):";

  /*
  for (const auto& range_query : range_query_map_) {
    ret += " " + range_query.first + ": ";
    ret += (range_query.second.lower_open) ? "(" : "[";
    ret += range_query.second.lower + ", ";
    ret += range_query.second.upper;
    ret += (range_query.second.upper_open) ? ")" : "]";
  }
  */
  return ret;
}

const StringRange* ComplexQuery::range_query(const string &name) const {
  return find_or_null(range_query_map_, name);
}

vector<string> ComplexQuery::get_names_of_range_queries() const {
  vector<string> results;
  append_keys_from_map(range_query_map_, &results);
  return results;
}

}  // namespace vsfs
