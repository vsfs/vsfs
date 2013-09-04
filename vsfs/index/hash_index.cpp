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

#include <stdint.h>
#include <string>
#include "vobla/traits.h"
#include "vsfs/index/hash_index.h"

using std::string;

namespace vsfs {
namespace index {

HashIndexInterface* HashIndexInterface::create_hash_index(int key_type) {
  switch (key_type) {
    case TypeIDs::UINT64:
      return new HashIndex<uint64_t>();
    case TypeIDs::INT64:
      return new HashIndex<int64_t>();
    case TypeIDs::FLOAT:
      return new HashIndex<float>();
    case TypeIDs::DOUBLE:
      return new HashIndex<double>();
    case TypeIDs::STRING:
      return new HashIndex<std::string>();
    case TypeIDs::INT32:
      return new HashIndex<int32_t>();
    case TypeIDs::UINT32:
      return new HashIndex<uint32_t>();
    case TypeIDs::INT16:
      return new HashIndex<int16_t>();
    case TypeIDs::UINT16:
      return new HashIndex<uint16_t>();
    case TypeIDs::CHAR:
      return new HashIndex<char>();
    case TypeIDs::INT8:
      return new HashIndex<int8_t>();
    case TypeIDs::UINT8:
      return new HashIndex<uint8_t>();
    default:
      return nullptr;
  }
}

}  // namespace index
}  // namespace vsfs
