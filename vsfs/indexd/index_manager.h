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

#ifndef VSFS_INDEXD_INDEX_MANAGER_H_
#define VSFS_INDEXD_INDEX_MANAGER_H_

#include <gtest/gtest_prod.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "vobla/status.h"
#include "vobla/macros.h"
#include "vsfs/common/log_manager.h"
#include "vsfs/index/index_info.h"
#include "vsfs/index/range_index.h"

using std::condition_variable;
using std::mutex;
using std::queue;
using std::string;
using std::thread;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace vsfs {

class RpcComplexQuery;
class RpcIndexInfo;
class RpcIndexInfoRequest;
class RpcIndexRecordUpdateList;
class RpcIndexUpdate;
class RpcRangeQuery;

namespace index {
class IndexInfo;
}

namespace indexd {

using index::RangeIndexInterface;
using index::IndexInfo;

/**
 * \class IndexRecordUpdateOp
 * \brief The operator of updating a record.
 */
class IndexRecordUpdateOp {
 public:
  enum {
    UNKNOWN,
    /// Inserts an record into the index
    INSERT,
    /// Updates an record with new value.
    UPDATE,
    /// Removes an record.
    REMOVE,
  };

  IndexRecordUpdateOp(int opcode, const string &k, const string &v)
      : op_(opcode), key_(k), value_(v) {
  }

  ~IndexRecordUpdateOp() {}

  /// Returns the operation code.
  int op() const { return op_; }

  /// Returns the string representation of the index key.
  const string& key() const { return key_; }

  /// Returns the string representation of the index value.
  const string& value() const { return value_; }

 private:
  IndexRecordUpdateOp();

  /// Operation code.
  int op_;
  /// The string representation of index key.
  string key_;
  /// The string representation of index value.
  string value_;
};

/**
 * \class IndexManager
 * \brief The in-ram index manager.
 *
 * It manages creating, deleting and updating in-memory indices.
 *
 * TODO(eddyxu): abstract the interface to IndexManagerInterface class.
 */
class IndexManager {
 public:
  typedef IndexRecordUpdateOp RecordUpdateOp;
  typedef vector<RecordUpdateOp> RecordUpdateOpVector;

  explicit IndexManager(const string &basedir);

  virtual ~IndexManager();

  /**
   * \brief Creates an in-ram index.
   * \param txn_id transaction ID.
   * \param index_path the path of the index.
   * \param name the index name.
   * \param index_type the type of the index (e.g., BTREE or HASH).
   * \param key_type Optional, the key type of the index. (e.g., UINT64).
   *
   * \see "vobla/traits.h" for the definitions of possible key type values.
   */
  virtual Status create(uint64_t txn_id,
                        const string &index_path,
                        const string &name,
                        int index_type,
                        int key_type);

  /*
  virtual Status remove(uint64_t txn_id,
                        const string &index_path,
                        const string &name);
                        */

  /**
   * \brief Updates in-ram index.
   * \param txn_id the transaction ID.
   * \param index_path the path of the index.
   * \param name the name of the index.
   * \param updates a vector of RecordUpdateOp to be performed on this index.
   *
   * The content of 'updates' is cleared but the ownership of it is not
   * transferred to IndexManager.
   */
  virtual Status update(RpcIndexUpdate *updates);

  /// Searches files.
  virtual Status search(const RpcComplexQuery &query,
                        vector<int64_t>* results);

  virtual Status info(const RpcIndexInfoRequest &request,
                      RpcIndexInfo *info);

  /**
   * \brief Flushes all dirty indices to persistent storage.
   *
   * It should usually be called in a low-priority background thread.
   */
  virtual Status flush();

  RangeIndexInterface* get_range_index(const string &index_path,
                                       const string &name);

  Status merge_log_to_index(const string &index_path, uint64_t txn_id);

  /**
   * \brief Check every index if its size exceeds the partition threshold,
   * return the IndexInfo of the index if it needs partition.
   */
  Status find_indices_for_splitting(const size_t threshold,
                                    vector<IndexInfo*>* results);

 private:
  FRIEND_TEST(IndexManagerTest, TestCreateRangeIndex);
  FRIEND_TEST(IndexManagerTest, TestUpdateBasics);
  FRIEND_TEST(IndexManagerTest, TestUpdateOnlyAppendLogs);
  FRIEND_TEST(RangeIndexWrapperTest, TestDisallowMergeWhenMigrate);
  FRIEND_TEST(RangeIndexWrapperTest, TestFlushLog);

  /**
   * \brief Wraps a range index with more runtime information.
   *
   * \TODO(lxu): let it support all index type.
   */
  class RangeIndexWrapper {
    typedef LogManager<RpcIndexRecordUpdateList> LogManagerType;

   public:
    typedef LogManagerType::TxnIdType TxnIdType;

    /// The status of this range index.
    enum {
      NORMAL,   /**< normal status, can accept updates and merge requests. */
      MIGRATING, /**< this index is under migration, can not merge. */
    };

    /**
     * \brief Constructs a RangeIndexWrapper with an instance of RangeIndex.
     *
     * \note The ownership of 'index' is transferred to this class.
     */
    RangeIndexWrapper(const string &base_path, RangeIndexInterface *index);

    RangeIndexWrapper(const string &base_path,
                      RangeIndexInterface *index, bool dirty);

    ~RangeIndexWrapper();

    /**
     * \brief Applies updates to the index and append the updates into logs.
     *
     * The 'update' is not applied to the index immediately, instead, it only
     * appends 'update' to its internal log.
     *
     * \note The ownership of 'update' does not transferred to this class.
     *
     * The appended log should only being merged into the index when:
     *   - Calling search()
     *   - Calling merge()
     */
    Status update(TxnIdType txn_id, RpcIndexRecordUpdateList *update);

    Status search(TxnIdType txn_id, const RpcRangeQuery &query,
                  vector<uint64_t> *results);

    /**
     * \brief Explicitly merge the in-memory log to the index.
     *
     * \param txn_id Merges all updates having transaction ID <= txn_id.
     */
    Status merge(TxnIdType txn_id);

    const string& base_path() const {
      return base_path_;
    }

    /// Returns true if there are unstaged modifications.
    bool dirty() const;

    /// Returns the status of the
    int status() const;

    /// Sets a new status.
    void set_status(int new_status);

    /// Returns the RangeIndex instance.
    RangeIndexInterface* index() const;

    /**
     * \brief Flushes the log to the disk.
     *
     * \param force Sets it to true to force flushing the logs even there
     * are not enough logs in the buffer.
     */
    Status flush_log(bool force = false);

    /// Flushes the index to the disk.
    Status flush_index();

   private:
    string base_path_;

    /// Pointer to a RangeIndex instance.
    unique_ptr<RangeIndexInterface> index_;

    LogManagerType log_manager_;

    int status_;

    bool dirty_;

    mutex lock_;

    TxnIdType last_merged_log_id_;
  };

  index::IndexInfo* get_index_info(const string &index_info_key);

  Status update_single_index(
      RangeIndexWrapper::TxnIdType txn_id,
      const RpcIndexRecordUpdateList& updates);

  void background_flush_thread();

  /**
   * \brief Calculates the base file name for an index.
   *
   * It returns `basedir_/hash(path)`. Note that it does not include extension.
   * The caller needs to add '.log' or '.idx' to it.
   */
  string get_index_file_base_path(const string &path) const;

  string basedir_;

  /// A global mapping for fast looking up of the index type and key type.
  /// The key is the root path of the index.
  typedef unordered_map<string, index::IndexInfo> IndexInfoMap;

  IndexInfoMap index_info_map_;

  /// The mapping from 'index path' to the RangeIndex instsances.
  typedef unordered_map<string, unique_ptr<RangeIndexWrapper>> RangeIndexMap;

  RangeIndexMap range_index_map_;

  mutex lock_;

  /// Background flusher thread.
  thread bg_flusher_thread_;

  condition_variable bg_flusher_cv_;

  mutex bg_flusher_lock_;

  bool running_;

  DISALLOW_COPY_AND_ASSIGN(IndexManager);
};

}  // namespace indexd
}  // namespace vsfs

#endif  // VSFS_INDEXD_INDEX_MANAGER_H_
