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
 * \brief threads related functions.
 */

#ifndef VSFS_COMMON_THREAD_H_
#define VSFS_COMMON_THREAD_H_

#include <mutex>
#include <thread>
#include <vector>

using std::mutex;
using std::thread;
using std::vector;

typedef std::lock_guard<std::mutex> MutexGuard;

#endif  // VSFS_COMMON_THREAD_H_
