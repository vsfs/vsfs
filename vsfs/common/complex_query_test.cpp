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

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include "vobla/clock.h"
#include "vobla/range.h"
#include "vobla/status.h"
#include "vsfs/common/complex_query.h"

using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using vobla::FakeClock;
using vobla::Status;

namespace vsfs {

class ComplexQueryTest : public ::testing::Test {
 protected:
  void SetUp() {
    clock_.reset(new FakeClock);
    ComplexQuery::set_clock(clock_.get());
  }

  unique_ptr<FakeClock> clock_;
};

TEST_F(ComplexQueryTest, TestParseRoot) {
  string p("/a/?mtime<1d&size>=100");

  ComplexQuery q;
  EXPECT_TRUE(q.parse(p).ok());
  EXPECT_EQ("/a", q.root());

  auto rq = q.range_query("mtime");
  EXPECT_TRUE(rq != NULL);
  EXPECT_TRUE(rq->lower.empty());
  EXPECT_FALSE(rq->upper_closed);
  EXPECT_EQ(to_string(24*60*60), rq->upper);

  rq = q.range_query("size");
  EXPECT_TRUE(rq != NULL);
  EXPECT_TRUE(rq->upper.empty());
  EXPECT_TRUE(rq->lower_closed);
  EXPECT_EQ("100", rq->lower);

  string p1("/home/a/b/c/?size>1g");
  ComplexQuery q1;
  EXPECT_TRUE(q1.parse(p1).ok());
  EXPECT_EQ("/home/a/b/c", q1.root());
}

TEST_F(ComplexQueryTest, TestRelativeTimeInRangeQuery) {
  clock_->advance(1000);
  string p1("/g/?atime>-1");
  ComplexQuery q1;
  EXPECT_TRUE(q1.parse(p1).ok());

  string p2("/g/?mtime<+10");
  ComplexQuery q2;
  EXPECT_TRUE(q2.parse(p2).ok());
}

TEST_F(ComplexQueryTest, TestGlobalRangeQuery) {
  string p("/?size>10");
  ComplexQuery q;
  EXPECT_TRUE(q.parse(p).ok());
  EXPECT_EQ("/", q.root());
}

TEST_F(ComplexQueryTest, TestNamedRangeQuery) {
  string p("/home/john/?energy>=12.5");
  ComplexQuery q;
  EXPECT_TRUE(q.parse(p).ok());
  auto range = q.range_query("energy");
  EXPECT_TRUE(range != NULL);
  EXPECT_TRUE(range->lower_closed);
  EXPECT_EQ("12.5", range->lower);
  EXPECT_TRUE(range->upper.empty());

  EXPECT_TRUE(q.range_query("x-axis") == NULL);
}

TEST_F(ComplexQueryTest, TestQueryWithSpaces) {
  string path("/home/?energy >= 12.5");
  ComplexQuery query;
  EXPECT_TRUE(query.parse(path).ok());

  auto range = query.range_query("energy");
  EXPECT_TRUE(range != NULL);
  EXPECT_TRUE(range->lower_closed);
  EXPECT_EQ("12.5", range->lower);
  EXPECT_TRUE(range->upper.empty());
}

TEST_F(ComplexQueryTest, TestGetNamesOfRangeQueries) {
  string path("/home/?energy>12&mtime>100");
  ComplexQuery query;
  EXPECT_TRUE(query.parse(path).ok());

  vector<string> names = query.get_names_of_range_queries();
  EXPECT_EQ(2u, names.size());
}

}  // namespace vsfs
