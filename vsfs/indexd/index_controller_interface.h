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

#ifndef VSFS_INDEXD_INDEX_CONTROLLER_INTERFACE_H_
#define VSFS_INDEXD_INDEX_CONTROLLER_INTERFACE_H_

#include <string>
#include "vobla/status.h"

using std::string;
using vobla::Status;

namespace vsfs {
namespace indexd {

/**
 * \class IndexControllerInterface
 * \brief Interface for the index controller.
 */
class IndexControllerInterface {
 public:
  virtual ~IndexControllerInterface() {}

  /**
   * \brief Creates a file-index with a name.
   * \param root_path the path to create the file-index for.
   * \param name the name of the index.
   * \param index_type The type of the index (internal data structure). It
   * could be one of the BTREE/HASH or etc.
   * \param key_type the c++ type of the key.
   */
  virtual Status create_index(const string &root_path, const string &name,
                              int index_type, int key_type) = 0;
};

}  // namespace indexd
}  // namespace vsfs

#endif  // VSFS_INDEXD_INDEX_CONTROLLER_INTERFACE_H_
