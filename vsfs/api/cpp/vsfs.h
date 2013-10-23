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

#ifndef VSFS_API_CPP_VSFS_H_
#define VSFS_API_CPP_VSFS_H_

#include <memory>
#include "vobla/traits.h"

namespace vsfs {

class VsfsImpl;

/**
 * \class Vsfs
 * \brief Vsfs's C++ API.
 */
class Vsfs {
 public:
  enum class KeyType {
    CHAR = TypeIDs::CHAR,
    UINT8 = TypeIDs::UINT8,
    INT8 = TypeIDs::INT8,
    UINT16 = TypeIDs::UINT16,
    INT16 = TypeIDs::INT16,
    UINT32 = TypeIDs::UINT32,
    INT32 = TypeIDs::INT32,
    UINT64 = TypeIDs::UINT64,
    INT64 = TypeIDs::INT64,
    FLOAT = TypeIDs::FLOAT,
    DOUBLE = TypeIDs::DOUBLE,
    STRING = TypeIDs::STRING
  };

  Vsfs();

  virtual ~Vsfs();

 private:
  std::unique_ptr<VsfsImpl> impl_;
};

}  // namespace vsfs

#endif  // VSFS_API_CPP_VSFS_H_
