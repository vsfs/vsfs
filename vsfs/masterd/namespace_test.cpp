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
#include <errno.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "vsfs/common/test_leveldb_store.h"
#include "vsfs/masterd/namespace.h"

using ::testing::ContainerEq;
using ::testing::Contains;
using std::set;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
namespace fs = boost::filesystem;

namespace vsfs {
namespace masterd {

/**
 * \class NamespaceTest
 * \brief Unit tests for the unified namespace.
 */
class NamespaceTest : public ::testing::Test {
 protected:
  void SetUp() {
    test_store_ = new TestLevelDBStore;
    test_ns_.reset(new Namespace(test_store_));
  }

  /// Creates a file and add it to its parent directory's subfiles.
  void create_file(const string &path) {
    ObjectId obj;
    test_ns_->create(path, 0x666, 100, 100, &obj);
    auto fullpath = fs::path(path);
    test_ns_->add_subfile(fullpath.parent_path().string(),
                          fullpath.filename().string());
  }

  void create_dir(const string &path) {
    test_ns_->mkdir(path, 0x066, 100, 100);
    if (path != "/") {
      auto fullpath = fs::path(path);
      test_ns_->add_subfile(fullpath.parent_path().string(),
                            fullpath.filename().string());
    }
  }

  TestLevelDBStore* test_store_;
  unique_ptr<Namespace> test_ns_;
};

TEST_F(NamespaceTest, CreateFile) {
  ObjectId obj;
  string path = "/foo/bar";
  EXPECT_TRUE(test_ns_->create(path, 0x666, 100, 100, &obj).ok());

  ObjectId actual_obj;
  EXPECT_TRUE(test_ns_->object_id(path, &actual_obj).ok());
  EXPECT_EQ(obj, actual_obj);

  string actual_path;
  EXPECT_TRUE(test_ns_->file_path(obj, &actual_path).ok());
  EXPECT_EQ(path, actual_path);

  EXPECT_FALSE(test_ns_->create(path, 0x666, 100, 200, &obj).ok());
}

TEST_F(NamespaceTest, RemoveFile) {
  EXPECT_FALSE(test_ns_->remove("/foo/bar").ok());

  ObjectId obj;
  test_ns_->create("/foo", 0x666, 100, 200, &obj);
  string actual_path;
  EXPECT_TRUE(test_ns_->file_path(obj, &actual_path).ok());

  EXPECT_TRUE(test_ns_->remove("/foo").ok());
  EXPECT_FALSE(test_ns_->file_path(obj, &actual_path).ok());
  EXPECT_FALSE(test_ns_->object_id("/foo", &obj).ok());
}

TEST_F(NamespaceTest, TestMakeDirs) {
  EXPECT_TRUE(test_ns_->mkdir("/", 0x666, 100, 100).ok());
  EXPECT_TRUE(test_ns_->mkdir("/foo/bar", 0x666, 100, 100).ok());
  EXPECT_FALSE(test_ns_->mkdir("/foo/bar", 0x666, 100, 100).ok());
}

TEST_F(NamespaceTest, TestRemoveDirs) {
  EXPECT_FALSE(test_ns_->rmdir("/foo").ok());

  test_ns_->mkdir("/foo", 0x666, 100, 100);
  EXPECT_TRUE(test_ns_->rmdir("/foo").ok());

  test_ns_->mkdir("/foo", 0x666, 100, 100);
  test_ns_->add_subfile("/foo", "bar");
  // The directory is not empty.
  auto status = test_ns_->rmdir("/foo");
  EXPECT_EQ(-ENOTEMPTY, status.error());
}

TEST_F(NamespaceTest, TestAddSubFiles) {
  EXPECT_FALSE(test_ns_->add_subfile("/foo", "bar").ok());

  test_ns_->mkdir("/foo", 0x666, 100, 100);
  EXPECT_TRUE(test_ns_->add_subfile("/foo", "bar").ok());
  EXPECT_TRUE(test_ns_->add_subfile("/foo", "zoo").ok());
  EXPECT_FALSE(test_ns_->add_subfile("/foo", "bar").ok());

  vector<string> subfiles;
  EXPECT_TRUE(test_ns_->readdir("/foo", &subfiles).ok());
  EXPECT_EQ("bar", subfiles[0]);
  EXPECT_EQ("zoo", subfiles[1]);
}

TEST_F(NamespaceTest, TestInitialize) {
  create_dir("/");
  create_dir("/foo");
  create_dir("/foo/bar");
  for (int i = 0; i < 50; i++) {
    create_file(string("/foo/test") + std::to_string(i));
  }
  for (int i = 0; i < 50; i++) {
    create_file(string("/foo/bar/bla") + std::to_string(i));
  }

  TestLevelDBStore* new_store = new TestLevelDBStore(test_store_->store());
  Namespace new_ns(new_store);
  EXPECT_TRUE(new_ns.init().ok());

  vector<string> subfiles;
  EXPECT_TRUE(new_ns.readdir("/", &subfiles).ok());
  EXPECT_EQ("foo", subfiles[0]);

  subfiles.clear();
  EXPECT_TRUE(new_ns.readdir("/foo", &subfiles).ok());
  set<string> expected_files;
  for (int i = 0; i < 50; i++) {   // test0..test49 + bar
    expected_files.insert("test" + std::to_string(i));
  }
  expected_files.insert("bar");
  set<string> actual_files(subfiles.begin(), subfiles.end());
  EXPECT_THAT(actual_files, ContainerEq(expected_files));

  subfiles.clear();
  EXPECT_TRUE(new_ns.readdir("/foo/bar", &subfiles).ok());
  expected_files.clear();
  for (int i = 0; i < 50; i++) {  // bla0..bla49
    expected_files.insert("bla" + std::to_string(i));
  }
  actual_files.clear();
  actual_files.insert(subfiles.begin(), subfiles.end());
  EXPECT_THAT(actual_files, ContainerEq(expected_files));
}

TEST_F(NamespaceTest, TestFindFiles) {
  vector<string> expected_files;
  vector<ObjectId> object_ids;
  for (int i = 0; i < 10; i++) {
    string path = "/test" + to_string(i);
    ObjectId oid;
    EXPECT_TRUE(test_ns_->create(path, 0x666, 100, 100, &oid).ok());
    EXPECT_EQ(0, std::count(object_ids.begin(), object_ids.end(), oid));
    object_ids.push_back(oid);
    expected_files.push_back(path);
  }

  // Appends a few non-existed hash values.
  for (ObjectId i = 1; i < 10; i++) {
    if (std::count(object_ids.begin(), object_ids.end(), i)) {
      object_ids.emplace_back(i);
      expected_files.emplace_back("");
    }
  }

  vector<string> actual_files;
  EXPECT_TRUE(test_ns_->find_files(object_ids, &actual_files).ok());
  EXPECT_THAT(actual_files, ContainerEq(expected_files));
}

}  // namespace masterd
}  // namespace vsfs
