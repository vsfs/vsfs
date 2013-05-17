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

#include <glog/logging.h>
#include <string>
#include "vsfs/common/leveldb_store.h"

using leveldb::DB;
using leveldb::Iterator;

namespace vsfs {

LevelDBStore::LevelDBStoreIterator::LevelDBStoreIterator()
    : iter_(leveldb::NewEmptyIterator()) {
}

LevelDBStore::LevelDBStoreIterator::LevelDBStoreIterator(
    Iterator* iter) : iter_(iter) {
  iter_->SeekToFirst();
}

void LevelDBStore::LevelDBStoreIterator::increment() {
  if (iter_->Valid()) {
    iter_->Next();
  }
}

void LevelDBStore::LevelDBStoreIterator::decrement() {
  if (iter_->Valid()) {
    iter_->Prev();
  }
}

LevelDBStore::LevelDBStoreIterator::reference
LevelDBStore::LevelDBStoreIterator::dereference() const {
  CHECK(iter_->Valid());
  // TODO(eddyxu): these const_cast(s) are very ugly..
  const_cast<value_type*>(&key_and_value_)->first = iter_->key().ToString();
  const_cast<value_type*>(&key_and_value_)->second = iter_->value().ToString();
  return const_cast<value_type&>(key_and_value_);
}

bool LevelDBStore::LevelDBStoreIterator::equal(
    LevelDBStoreIterator const& other) const {
  return (!iter_->Valid()) && (!other.iter_->Valid());
}

LevelDBStore::LevelDBStore(const string &path) : db_path_(path) {
}

Status LevelDBStore::open() {
  leveldb::DB *db;
  leveldb::Options options;
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

LevelDBStore::iterator LevelDBStore::begin() {
  return LevelDBStoreIterator(db_->NewIterator(leveldb::ReadOptions()));
}

LevelDBStore::iterator LevelDBStore::end() {
  return LevelDBStoreIterator();
}
}  // namespace vsfs
