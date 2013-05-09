/* vim: ft=thrift:sw=2:sts=2
 *
 * VSFS Rpc Interfaces
 *
 * Note:
 *  - all Rpc structures have a prefix "Rpc" in the name.
 *  - A non-Rpc prefix is created also created for being used into the code
 *  transparently.
 */

// TODO(eddyxu): change the namespace to vsfs::rpc
namespace cpp vsfs

typedef i64 RpcFileId
typedef list<string> RpcFileList
typedef list<string> RpcKeywordList
typedef string RpcRawData

// Rpcepresents a File object in VSFS namespace.
// See common/file_info.h for more details.
struct RpcFileInfo {
  1: i64 id,
  2: string path,
  3: string uri,
  4: i64 uid,
  5: i64 gid,
  6: i64 mode,
  7: i64 size,
  8: i64 ctime,
  9: i64 mtime,
  10: i64 atime,
}

// See 'common/context.h' for more details.
struct RpcContext {
  1: string hostname,
  2: i32 pid,
  3: i32 ppid,
  4: i32 uid,
  5: i32 gid,
}

// IP address of a node.
struct RpcNodeAddress {
  1: string host,
  2: i32 port,
}

typedef list<RpcNodeAddress> RpcNodeAddressList
typedef RpcNodeAddressList NodeAddressList

// The machine information of GroupNode.
struct RpcNodeInfo {
  1: RpcNodeAddress address,
  2: string server_id,
  3: i16 type,   // MASTER or INDEXD, not used for now.
  4: i16 status,     /* Running status */
  5: i64 avail_mb,  /* Available space in MB */
}

/// NodeInfo is suggested used in the non-RPC code, and RpcNodeInfo is used
/// in the RPC-related code, such as IndexServerHandler.
typedef RpcNodeInfo NodeInfo

// Query type on named index.
enum NamedQueryType {
  CONTENT_QUERY,  // Content / Keyword query.
  RANGE_QUERY,  // Query on single range.
  MULTI_RANGE_QUERY,  // Query on a multi-dimentional tree.
}

struct RpcRangeQuery {
  1: required string index_path,
  2: required string name,
  3: required string lower,
  4: required bool lower_open,
  5: required string upper,
  6: required bool upper_open,
}

typedef list<RpcRangeQuery> RpcRangeQueryList

struct RpcComplexQuery {
  1: required i64 txn_id,
  2: required string path_prefix,
  3: required RpcRangeQueryList range_queries,
}

exception RpcInvalidOp {
  1: i32 what,
  2: string why,
}

// Operation on a named index
struct RpcIndexName {
  1: required string root_path,
  2: required string name,
}

struct RpcIndexKeyValue {
  1: required string root_path,
  2: required string name,
  3: required string key,
  4: optional string value,
}

// The operations on index records.
enum RpcIndexUpdateOpCode {
  UNKNOWN,
  INSERT,
  UPDATE,
  REMOVE,
}

// The update on a single record.
struct RpcIndexRecordUpdateOp {
  1: required RpcIndexUpdateOpCode op,  // operation code.
  2: required string key,  // the key to be opre
  3: optional string value,
}

struct RpcIndexRecordUpdateList {
  1: required string root_path,
  2: required string name,
  3: required list<RpcIndexRecordUpdateOp> record_updates,
}

// The RPC package to update an index record.
struct RpcIndexUpdate {
  1: required i64 txn_id,
  2: required list<RpcIndexRecordUpdateList> updates,
}

struct RpcIndexCreateType {
  1: required string root_path,
  2: required string name,
  3: required i32 index_type,
  4: optional i32 key_type,
}

struct RpcIndexLocation {
  1: required string full_index_path,
  2: required RpcNodeAddress server_addr,
  3: required list<i64> file_ids
}

typedef list<RpcIndexLocation> RpcIndexLocationList

/**
 * \brief Describes a Consistent Hash Ring for all the partitions of one index.
 */
struct RpcIndexPartitionRing {
  1: required string root_path,
  2: required string index_name,
  3: required map<i64, RpcNodeAddress> partition_locations,
}

typedef list<RpcIndexPartitionRing> RpcIndexPartitionRingList

struct RpcMetaLocation {
  1: required i64 file_id,
  2: required RpcNodeAddress server_addr,
}

typedef list<RpcMetaLocation> RpcMetaLocationList

struct RpcMetaData {
  1: required i64 file_id,
  2: required string file_path;
}

typedef list<RpcMetaData> RpcMetaDataList

/**
 * \brief Lookup request for index.
 *
 * It works in two ways:
 *  - No cached (cached = false), the optional field `dir_to_file_id_map` is
 *    used. This field is a map between "dir names" -> "vector<hash(file
 *    path)>".
 *  - Cached (cached = true), only 'dirs' is set. The 'dirs' is the common
*     directories used by all files to index.
 */
struct RpcIndexLookupRequest {
  1: required string name,  // Index name
  2: required bool cached,
  3: optional map<string, list<i64>> dir_to_file_id_map,
  4: optional list<string> dirs;
}

struct RpcIndexInfoRequest {
  1: required i64 txn_id,
  2: required string path,
  3: optional string name,
  4: optional bool recursive,
}

/**
 * \brief Encapsures the detailed information of every single Index.
 */
struct RpcIndexInfo {
  1: required string path,
  2: required string name,
  3: required i32 type,  // index type
  4: required i32 key_type,
  5: required RpcIndexLocationList locations,
  6: required i64 num_records,  // Number of records.
}

/*
 * \brief Index Metadata along with the serealized index data.
 */
struct RpcIndexMigrationData {
  1: required RpcIndexInfo idx_info,
  2: required RpcRawData raw_data,  // Serealized Index Data
}

typedef list<RpcIndexInfo> RpcIndexInfoList

/**
 * \brief MasterDaemon, the centralized coordinartor for VSFS cluster.
 */
service MasterDaemon {
  /**
   * \brief An index server join the hash ring.
   *
   * \return RpcNodeAddressList a list of replica servers for this index
   * server.
   */
  RpcNodeAddressList join_index_server(1:RpcNodeInfo info);

  /**
   * \brief A Meta Server join the hash ring.
   * \return RpcNodeAddressList a list of replica Meta Servers.
   */
  RpcNodeAddressList join_meta_server(1:RpcNodeInfo info);

  void disjoin(1:RpcNodeInfo info);

  /**
   * \brief Add a new index partition to index partition CH ring after a
   * server completed index migration and created a new index partition.
   * \param[in] idx_info basic info of the index.
   * \param[in] sep the start position of the new partition.
   */
  void add_index_partition(1:RpcIndexInfo idx_info, 2:i64 sep)
	throws (1:RpcInvalidOp ouch);

  /// Get the next available ID.
  i64 get_txn_id();

  /**
   * \brief Acquires 'num_ids' of Txn Ids.
   * \param num_ids the total number of Txn IDs to ask.
   * \return the first ID of the 'num_ids' IDs.
   */
  i64 get_txn_ids(1:i64 num_ids);

  RpcNodeAddressList search(1:RpcComplexQuery query);

  /**
   * \brief Creates an index and assign it to a IndexServer in the hash
   * ring.
   * \return the address of IndexServer to manages this index.
   */
  RpcIndexLocation create_index(1:RpcIndexCreateType index)
	throws (1:RpcInvalidOp ouch);

  /// Locates index servers for files.
  RpcIndexLocationList locate_index(1: RpcIndexLookupRequest lookup)
	throws (1:RpcInvalidOp ouch);

  /// Locates all partitions of each index.
  RpcIndexPartitionRingList locate_partitions(1: RpcIndexLookupRequest lookup)
	throws (1:RpcInvalidOp ouch);

  /**
   * \brief Returns the index paths for all index that have names in
   * `index_names`, which also includes files under 'root_path'.
   */
  RpcIndexLocationList locate_index_for_search(
	  1:string root_path, 2:list<string> index_names)
	throws (1:RpcInvalidOp ouch);

  /**
   * \brief Locate MetaServer for file_id, for each file_id, return a
   * (file_id, server) pair.
   */
  RpcMetaLocationList locate_metadata(
	  1:list<i64> file_ids) throws (1:RpcInvalidOp ouch);

  /**
   * \brief locate the RpcNodeAddress of this file.
   * \param path the absolute path in VSFS.
   */
  RpcNodeAddress locate(1:string path) throws (1:RpcInvalidOp ouch);

  RpcNodeAddress create(1:string group_id) throws (1:RpcInvalidOp ouch);

  /**
   * \brief Index Migration phase 1.
   */
  RpcNodeAddress get_new_index_partition_addr(
	  1:i64 split_pos, 2:RpcIndexInfo idx_info, 3:RpcNodeAddress giver_node)
	throws (1:RpcInvalidOp ouch);

  /**
   * \brief Query the index info on master node.
   */
  RpcIndexInfoList info(1:RpcIndexInfoRequest request)
	throws (1:RpcInvalidOp ouch);
}

/**
 * Index Server.
 *
 * Managers the RPC communications between client and index server.
 */
service IndexServer {
  /**
   * \brief Creates an index.
   */
  void create_index(1:RpcIndexCreateType index)
	  throws (1:RpcInvalidOp ouch);

  /**
   * \Remove an entire index.
   */
  void remove_index(1:RpcIndexName name) throws (1:RpcInvalidOp ouch);

  void update(1:RpcIndexUpdate updates) throws (1:RpcInvalidOp ouch);

  list<RpcFileId> search(1:RpcComplexQuery query)
	throws (1:RpcInvalidOp ouch);

  /**
   * \brief Queries information for a single index partition.
   */
  RpcIndexInfo info(1:RpcIndexInfoRequest request)
	throws (1:RpcInvalidOp ouch);

  /*--------------------------Migration Operations---------------------------*/

  /**
   * \brief Migrate data from another index server.
   */
  void migrate(1:RpcIndexMigrationData data)
	throws (1:RpcInvalidOp ouch);

  /**
   * \brief Transfer the updates only to the log of the remote machines.
   * \note this function should only be called during Migration Phase 1
   * for request forwarding.
   */
  void update_to_remote_log(1:RpcIndexUpdate updates)
	throws (1:RpcInvalidOp ouch);

  /**
   * \brief Let taker node officially join the Index Server CH ring.
   */
  void join_taker_node_server() throws (1:RpcInvalidOp ouch);

  /**
   * \brief Let taker node add the new index partition to index partition
   * CH ring.
   */
  void join_taker_node_index_partition(1:RpcIndexInfo idx_info, 2:i64 sep)
	throws (1:RpcInvalidOp ouch);
}

/**
 * Metadata server.
 */
service MetaServer {
  void insert(1:RpcMetaDataList metadata) throws (1:RpcInvalidOp ouch);

  void remove(1:i64 file_id) throws (1:RpcInvalidOp ouch);

  string find(1:i64 file_id) throws (1:RpcInvalidOp ouch);

  /**
   * \brief Search by the given file_ids, return a list of file_paths.
   */
  list<string> search(1:list<i64> file_ids);
}
