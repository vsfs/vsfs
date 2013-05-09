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

#include <string>
#include "vsfs/index/range_index.h"

using std::string;

namespace vsfs {
namespace index {

RangeIndexInterface* RangeIndexInterface::create_range_index(int key_type) {
  switch (key_type) {
    case TypeIDs::UINT64:
      return new RangeIndex<uint64_t>();
    case TypeIDs::INT64:
      return new RangeIndex<int64_t>();
    case TypeIDs::FLOAT:
      return new RangeIndex<float>();
    case TypeIDs::DOUBLE:
      return new RangeIndex<double>();
    case TypeIDs::STRING:
      return new RangeIndex<std::string>();
    case TypeIDs::INT32:
      return new RangeIndex<int32_t>();
    case TypeIDs::UINT32:
      return new RangeIndex<uint32_t>();
    case TypeIDs::INT16:
      return new RangeIndex<int16_t>();
    case TypeIDs::UINT16:
      return new RangeIndex<uint16_t>();
    case TypeIDs::CHAR:
      return new RangeIndex<char>();
    case TypeIDs::INT8:
      return new RangeIndex<int8_t>();
    case TypeIDs::UINT8:
      return new RangeIndex<uint8_t>();
    default:
      return nullptr;
  }
}

}  // namespace index
}  // namespace vsfs
