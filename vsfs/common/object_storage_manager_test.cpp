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

#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "vobla/file.h"
#include "vobla/status.h"
#include "vsfs/common/file_object.h"
#include "vsfs/common/object_storage_manager.h"

using ::testing::ElementsAre;
using std::sort;
using std::string;
using std::unique_ptr;
using std::vector;
using vobla::TemporaryDirectory;
namespace fs = boost::filesystem;

namespace vsfs {

class ObjectStorageManagerTest : public ::testing::Test {
 protected:
  void SetUp() {
    tmpdir_.reset(new TemporaryDirectory);
  }

  unique_ptr<TemporaryDirectory> tmpdir_;
};

TEST_F(ObjectStorageManagerTest, TestInit) {
  int subdirs = 8;
  ObjectStorageManager osm(tmpdir_->path(), subdirs);
  EXPECT_TRUE(osm.init().ok());
  vector<string> sub_names;
  for (auto dit = fs::directory_iterator(tmpdir_->path());
       dit != fs::directory_iterator(); ++dit) {
    sub_names.push_back(dit->path().filename().string());
  }
  sort(sub_names.begin(), sub_names.end());
  EXPECT_THAT(sub_names, ElementsAre("0", "1", "2", "3", "4", "5", "6", "7"));
}

TEST_F(ObjectStorageManagerTest, TestCreateFile) {
  int subdirs = 8;
  ObjectStorageManager osm(tmpdir_->path(), subdirs);
  EXPECT_TRUE(osm.init().ok());

  FileObject* fobj;
  EXPECT_TRUE(osm.open("/abcdef", 12, O_CREAT, 0666, &fobj).ok());
  fobj->close();
  delete fobj;

  EXPECT_TRUE(fs::exists(tmpdir_->path() + "/4/12"));
}

}  // namespace vsfs
