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

#include <glog/logging.h>
#include <map>
#include <memory>
#include <string>
#include "vsfs/common/leveldb_store.h"

using leveldb::DB;
using leveldb::Iterator;
using std::map;
using std::string;
using std::unique_ptr;

namespace vsfs {

LevelDBStore::LevelDBStoreIterator::LevelDBStoreIterator()
    : iter_(leveldb::NewEmptyIterator()) {
}

LevelDBStore::LevelDBStoreIterator::LevelDBStoreIterator(
    Iterator* iter) : iter_(iter) {
}

LevelDBStore::LevelDBStoreIterator::LevelDBStoreIterator(
    map<string, string>::iterator iter) : test_iter_(iter) {
}

void LevelDBStore::LevelDBStoreIterator::increment() {
  if (iter_) {
    if (iter_->Valid()) {
      iter_->Next();
    }
  } else {
    ++test_iter_;
  }
}

void LevelDBStore::LevelDBStoreIterator::decrement() {
  if (iter_) {
    if (iter_->Valid()) {
      iter_->Prev();
    }
  } else {
    ++test_iter_;
  }
}

LevelDBStore::LevelDBStoreIterator::reference
LevelDBStore::LevelDBStoreIterator::dereference() const {
  if (iter_) {
    CHECK(iter_->Valid());
    // TODO(eddyxu): these const_cast(s) are very ugly..
    const_cast<value_type*>(&key_and_value_)->first = iter_->key().ToString();
    const_cast<value_type*>(&key_and_value_)->second =
        iter_->value().ToString();
  } else {
    const_cast<value_type*>(&key_and_value_)->first = test_iter_->first;
    const_cast<value_type*>(&key_and_value_)->second = test_iter_->second;
  }
  return const_cast<value_type&>(key_and_value_);
}

string LevelDBStore::LevelDBStoreIterator::key() const {
  if (iter_->Valid()) {
    return iter_->key().ToString();
  }
  return "";
}

string LevelDBStore::LevelDBStoreIterator::value() const {
  if (iter_->Valid()) {
    return iter_->value().ToString();
  }
  return "";
}

bool LevelDBStore::LevelDBStoreIterator::equal(
    LevelDBStoreIterator const& other) const {
  if (iter_) {
    return (!iter_->Valid()) && (!other.iter_->Valid());
  } else {
    return test_iter_ == other.test_iter_;
  }
}

bool LevelDBStore::LevelDBStoreIterator::starts_with(
    const string& prefix) const {
  return iter_ && iter_->Valid() && iter_->key().starts_with(prefix);
}

LevelDBStore::LevelDBStore(const string &path, int bufsize)
    : db_path_(path), bufsize_mb_(bufsize) {
}

Status LevelDBStore::open() {
  const int kMB = 1024 * 1024;
  leveldb::DB *db;
  leveldb::Options options;
  options.create_if_missing = true;
  options.write_buffer_size = bufsize_mb_ * kMB;
  leveldb::Status status = leveldb::DB::Open(options, db_path_, &db);
  if (!status.ok()) {
    return to_status(status);
  }
  db_.reset(db);
  return Status::OK;
}

Status LevelDBStore::create() {
  leveldb::DB *db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, db_path_, &db);
  if (!status.ok()) {
    return to_status(status);
  }
  db_.reset(db);
  return Status::OK;
}

Status LevelDBStore::get(const string& key, string* value) {
  leveldb::Status s = db_->Get(leveldb::ReadOptions(), key, value);
  return to_status(s);
}

Status LevelDBStore::put(const string& key, const string &value) {
  leveldb::Status s = db_->Put(leveldb::WriteOptions(), key, value);
  return to_status(s);
}

Status LevelDBStore::remove(const string& key) {
  leveldb::Status s = db_->Delete(leveldb::WriteOptions(), key);
  return to_status(s);
}

Status LevelDBStore::to_status(const leveldb::Status& l_status) const {
  if (l_status.ok()) {
    return Status::OK;
  }
  return Status(-1, l_status.ToString());
}

LevelDBStore::iterator LevelDBStore::search(const string& prefix) {
  unique_ptr<leveldb::Iterator> iter(db_->NewIterator(leveldb::ReadOptions()));
  iter->Seek(prefix);
  return LevelDBStoreIterator(iter.release());
}

LevelDBStore::iterator LevelDBStore::begin() {
  unique_ptr<leveldb::Iterator> iter(db_->NewIterator(leveldb::ReadOptions()));
  iter->SeekToFirst();
  return LevelDBStoreIterator(iter.release());
}

LevelDBStore::iterator LevelDBStore::end() {
  return LevelDBStoreIterator();
}

}  // namespace vsfs
