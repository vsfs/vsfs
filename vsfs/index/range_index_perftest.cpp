/**
 * \file range_index_perftest.cpp
 *
 * \brief in-memory performance test for RangeIndex.
 *
 * Sensitive studies of the RangeIndex performance characteristics.
 *
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
 */

#include <gflags/gflags.h>
#include <time.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include "vobla/timer.h"
#include "vsfs/index/range_index.h"

using std::multiset;
using std::unique_ptr;
using vobla::Timer;
using vsfs::index::RangeIndex;
using vsfs::index::RangeIndexInterface;

DEFINE_uint64(num_records, 1000000,
              "Sets the total record in the RangeIndex.");
DEFINE_double(result_ratio, 0.1, "Sets the ratio of returned results in the "
              "total records.");

unsigned seed;

/**
 * \Returns the results that are the given ratio of the total_records.
 */
void search_results_for_ratio() {
  RangeIndex<uint64_t> index;
  multiset<uint64_t> keys;
  for (uint64_t i = 0; i < FLAGS_num_records; ++i) {
    uint64_t key = rand_r(&seed);
    index.insert(key, i);
    keys.insert(key);
  }
  uint64_t num_returned = FLAGS_num_records * FLAGS_result_ratio;
  uint64_t start_key_pos = rand_r(&seed) %
      static_cast<uint64_t>((FLAGS_num_records * (1 - FLAGS_result_ratio)));
  auto iter = keys.begin();
  std::advance(iter, start_key_pos);
  uint64_t low_key = *iter;
  std::advance(iter, num_returned);
  CHECK(iter != keys.end());
  uint64_t high_key = *iter + 1;

  RangeIndexInterface::FileIdVector files;
  files.reserve(num_returned);
  Timer timer;
  timer.start();
  index.search(low_key, false, high_key, false, &files);
  timer.stop();

  double latency = timer.get_in_ms();
  printf("# Files\tRatio\tLatency(ms)\n%lu\t%0.2f\t%0.2f\n",
         FLAGS_num_records, FLAGS_result_ratio, latency);
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  seed = time(NULL);

  search_results_for_ratio();

  return 0;
}
