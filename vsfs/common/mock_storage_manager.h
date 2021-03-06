/*
 * Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_COMMON_MOCK_STORAGE_MANAGER_H_
#define VSFS_COMMON_MOCK_STORAGE_MANAGER_H_

#include <gmock/gmock.h>
#include <string>
#include "vsfs/common/storage_manager.h"

namespace vsfs {

class MockStorageManager : public StorageManager {
 public:
  MOCK_METHOD0(init, Status());
  MOCK_METHOD0(destroy, Status());
  MOCK_METHOD4(open, Status(const string&, ObjectId, int, File**));
  MOCK_METHOD5(open, Status(const string&, ObjectId, int, mode_t,
                            File**));
  MOCK_METHOD2(unlink, Status(const string&, ObjectId));
  MOCK_METHOD2(mkdir, Status(const string&, mode_t));
  MOCK_METHOD1(rmdir, Status(const string&));
  MOCK_METHOD5(readlink, Status(const string&, ObjectId, char*,
                                size_t, ssize_t*));
  MOCK_METHOD3(getattr, Status(const string&, ObjectId, struct stat*));
  MOCK_METHOD1(statfs, Status(struct statvfs*));
  MOCK_METHOD3(symlink, Status(const string&, const string&, ObjectId));
};

}  // namespace vsfs

#endif  // VSFS_COMMON_MOCK_STORAGE_MANAGER_H_
