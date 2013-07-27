/*
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
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
 * \file types.h "vsfs/common/types.h"
 * \brief Defines the common types used by all VSFS components.
 */

#ifndef VSFS_COMMON_TYPES_H_
#define VSFS_COMMON_TYPES_H_

#include <cstdint>

namespace vsfs {

typedef int64_t ObjectId;
typedef int64_t FilePathHashType;
// TODO(eddyxu): Merge HashValueType and FilePathHashType together.
typedef int64_t HashValueType;

}  // namespace vsfs

#endif  // VSFS_COMMON_TYPES_H_
