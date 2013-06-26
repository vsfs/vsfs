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

#ifndef VSFS_COMMON_LOG_MANAGER_H_
#define VSFS_COMMON_LOG_MANAGER_H_

#include <glog/logging.h>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "vobla/macros.h"
#include "vobla/status.h"
#include "vsfs/common/log_types.h"
#include "vsfs/common/thread.h"
#include "vsfs/rpc/thrift_utils.h"

using std::list;
using std::mutex;
using std::string;
using std::unique_ptr;
using std::vector;
using vobla::Status;

namespace vsfs {

/**
 * \brief A single Log record.
 */
template <typename TxnId, typename Record>
class LogRecord {
 public:
  typedef TxnId TxnIdType;
  typedef Record RecordType;

  /// Constructs an empty log record.
  LogRecord() : txn_id_(0) {
  }

  /**
   * \brief Constructs a log record with transaction id and data.
   * \param txn_id the transaction id of this record.
   * \param data the buffer that contains the changes. Note that the owernship
   * of it is transferred to LogRecord.
   */
  LogRecord(TxnIdType txn, Record *rec) : txn_id_(txn), record_(rec) {
  }

  ~LogRecord() {}

  /// Returns the transaction ID.
  TxnIdType txn_id() const {
    return txn_id_;
  }

  void set_txn_id(TxnIdType txn) {
    txn_id_ = txn;
  }

  /// Returns the pointer of the log record.
  const RecordType* record() const {
    return record_.get();
  }

  /// Transforms the record to a string buffer.
  string serialize() const {
    return ThriftUtils::serialize(*record_);
  }

  Status parse(const string &buf) {
    record_.reset(new RecordType);
    if (!ThriftUtils::deserialize(buf, record_.get())) {
      return Status(-1, "Failed to parse Log.");
    }
    return Status::OK;
  }

 private:
  TxnIdType txn_id_;

  unique_ptr<RecordType> record_;
};

/**
 * \class LogManager
 * \brief Manages the log of updates.
 *
 * \note This LogManager is thread-safe.
 */
template <typename R>
class LogManager {
 public:
  typedef uint64_t TxnIdType;

  typedef R RecordType;

  typedef LogRecord<TxnIdType, RecordType> LogRecordType;

  /// Default constructor.
  LogManager() : last_flushed_txn_id_(0) {
  }

  virtual ~LogManager() {
  }

  /**
   * \brief Appends a log buffer with a transaction ID.
   * \param txn_id transaction ID.
   * \param buf the buffer that contains the log entry. The ownership of 'buf'
   * is transferred to this LogManager.
   *
   * \note transaction ID must be unique and ordered.
   */
  Status append(TxnIdType txn_id, RecordType *record) {
    CHECK_NOTNULL(record);
    MutexGuard guard(mutex_);
    if (!log_.empty()) {
      // Checks whether the log is sorted.
      // TODO(eddyxu): need to handle the txn_id overflow later.
      /*
      if (log_.back().txn_id() >= txn_id) {
        return Status(-1, "The transaction ID is invalid");
      }
      */
    }
    log_.emplace_back(txn_id, record);
    return Status::OK;
  }

  /// Erases all records that have txn id less than or equal to txn_id.
  void erase(TxnIdType txn_id) {
    MutexGuard guard(mutex_);
    auto it = log_.begin();
    for (; it != log_.end() && it->txn_id() <= txn_id; ++it) {
    }
    log_.erase(log_.begin(), it);
  }

  /**
   * \brief Scans the log and puts the first N log records whose total space is
   * approximately equal to the 'bytes' into 'records'
   * \param[in] txn_id scans all records that have txn_id <= 'txn_id'.
   * \param[out] records filled with the scanned records.
   *
   * \note It is guaranteed that the total space of the first N-1 log records
   * is less than 'bytes', while the total space of the first N log records is
   * equal to or larger than 'bytes'.
   */
  Status scan(TxnIdType txn_id, vector<const RecordType*> *records) {
    CHECK_NOTNULL(records);
    MutexGuard guard(mutex_);
    typename LogType::iterator it = log_.begin();
    for (; it != log_.end() && it->txn_id() <= txn_id; ++it) {
      records->push_back(it->record());
    }
    return Status::OK;
  }

  /**
   * \brief Flushes the first nbytes to ostream.
   * \param bytes Only flushes (approximately) bytes of logs to os.
   * \param os the output stream for log flushing.
   * \param flush_if_not_full Sets to true to flush the log even there is no
   * enough records.
   */
  Status flush(size_t bytes, std::ostream *os, bool flush_if_not_full) {
    CHECK_NOTNULL(os);
    {
      MutexGuard guard(mutex_);
      auto it = log_.begin();
      if (last_flushed_txn_id_) {
        // Skips the logs that has already been flushed.
        while (it != log_.end() && it->txn_id() <= last_flushed_txn_id_) {
          ++it;
        }
      }
      size_t existing_bytes = get_log_buffer_size();
      for (; it != log_.end(); ++it) {
        LogEntity entity;
        entity.txn_id = it->txn_id();
        entity.data = std::move(it->serialize());
        log_buffer_.buffer.emplace_back(entity);
        existing_bytes += sizeof(entity.txn_id) + entity.data.size();
        if (existing_bytes >= bytes) {
          break;
        }
      }
      if (existing_bytes >= bytes || flush_if_not_full) {
        last_flushed_txn_id_ = log_buffer_.buffer.back().txn_id;

        // TODO(eddyxu): puts the IO to a background queue to make sure the
        // order between multiple threads calling flush simultaneously.
        string buffer = ThriftUtils::serialize(log_buffer_);
        *os << buffer.size();
        *os << buffer;
        log_buffer_.buffer.clear();
      }
    }
    return Status::OK;
  }

  /**
   * \brief Loads all logs from the stream.
   *
   * \note It erases all existing logs in this manager.
   */
  Status load(std::istream *is) {
    CHECK_NOTNULL(is);
    // TODO(eddyxu): Simply putting the new record back to 'log_' can not
    // ensure that the elements in the 'log_' are still in-ordered.
    log_.clear();

    LogBuffer log_buffer;

    while (is->good()) {
      size_t message_size;
      *is >> message_size;
      if (!is->good()) {
        break;
      }
      string buffer;
      buffer.resize(message_size);
      is->read(&buffer[0], message_size);
      if (!is->good()) {
        break;
      }
      log_buffer.buffer.clear();
      if (!ThriftUtils::deserialize(buffer, &log_buffer)) {
        LOG(ERROR) << "Failed to parse log from string.";
        return Status(-1, "Failed to parse log from stream.");
      }
      for (const auto& entity : log_buffer.buffer) {
        log_.emplace_back();
        log_.back().set_txn_id(entity.txn_id);
        log_.back().parse(entity.data);
      }
    }
    return Status::OK;
  }

  /// Returns the number of records in the log.
  size_t size() {
    MutexGuard guard(mutex_);
    return log_.size();
  }

  /// Returns the total space consumed by the log.
  size_t bytes();

  /// Returns the transaction ID of the last flushed log.
  TxnIdType last_flushed_txn_id() {
    return last_flushed_txn_id_;
  }

 private:
  size_t get_log_buffer_size() {
    size_t ret = 0;
    for (const auto& log_entity : log_buffer_.buffer) {
      ret += sizeof(log_entity.txn_id);
      ret += log_entity.data.size();
    }
    return ret;
  }

  typedef list<LogRecordType> LogType;

  LogType log_;

  LogBuffer log_buffer_;

  TxnIdType last_flushed_txn_id_;

  std::mutex mutex_;

  DISALLOW_COPY_AND_ASSIGN(LogManager);
};

}  // namespace vsfs

#endif  // VSFS_COMMON_LOG_MANAGER_H_
