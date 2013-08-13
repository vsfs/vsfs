/* vim: ft=thrift:sw=2:sts=2
 *
 * VSFS Rpc Interfaces
 *
 * Note:
 *  - all Rpc structures have a prefix "Rpc" in the name.
 *  - A non-Rpc prefix is created also created for being used into the code
 *  transparently.
 */

namespace cpp vsfs
namespace py vsfs
namespace java vsfs

typedef i64 RpcObjectId
typedef list<RpcObjectId> RpcObjectList
typedef list<string> RpcFileList
typedef list<string> RpcKeywordList
typedef string RpcRawData

exception RpcInvalidOp {
  1: i32 what,
  2: string why,
}

// Rpcepresents a File object in VSFS namespace.
// See common/file_info.h for more details.
struct RpcFileInfo {
  1: i64 id,
  2: string path,
  4: i64 uid,
  5: i64 gid,
  6: i64 mode,
  7: i64 size,
  8: i64 ctime,
  9: i64 mtime,
  10: i64 atime,
}

// IP address of a node.
struct RpcNodeAddress {
  1: string host,
  2: i32 port,
}

typedef list<RpcNodeAddress> RpcNodeAddressList
typedef RpcNodeAddressList NodeAddressList
typedef map<i64, RpcNodeAddress> RpcConsistentHashRing

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
  2: required string root,
  3: required RpcRangeQueryList range_queries,
}


// Operation on a named index
struct RpcIndexName {
  1: required string root,
  2: required string name,
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

/// The request of creating a file index.
struct RpcIndexCreateRequest {
  1: required string root,  // The root path of the index scope.
  2: required string name,  // The name of this index.
  3: required i32 index_type,  // The data structure of this index.
  4: required i32 key_type,  // The key type of this index.
  5: required i64 mode,
  6: required i64 uid,
  7: required i64 gid,
}

/**
 * \brief Describes a Consistent Hash Ring for all the partitions of one index.
 */
struct RpcIndexPartitionRing {
  1: required string root_path,
  2: required string index_name,
  3: required map<i64, RpcNodeAddress> partition_locations,
}

typedef list<RpcIndexPartitionRing> RpcIndexPartitionRingList

struct RpcIndexInfoRequest {
  1: required i64 txn_id,
  2: required string path,
  3: optional bool recursive
}

/**
 * \brief Encapsures the detailed information of every single Index.
 */
struct RpcIndexInfo {
  1: required string path,
  2: required string name,
  3: required i32 type,  // index type
  4: required i32 key_type,
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
 * \brief MasterServer, the centralized coordinartor for VSFS cluster.
 */
service MasterServer {
  /// A secondary master join the primary master.
  void join_master_server(1:RpcNodeInfo info) throws (1:RpcInvalidOp ouch);

  /**
   * \brief An index server joins the hash ring.
   * \return RpcNodeAddressList a list of replica servers for this index
   * server.
   */
  RpcNodeAddressList join_index_server(1:RpcNodeInfo info);

  /// Obtains the C.H ring of all masters.
  RpcConsistentHashRing get_all_masters() throws (1:RpcInvalidOp ouch);

  /// Obtains the C.H ring of all index servers.
  RpcConsistentHashRing get_all_index_servers() throws (1:RpcInvalidOp ouch);

  /**
   * \brief Makes a new directory.
   */
  void mkdir(1:string path, 2:RpcFileInfo info) throws (1:RpcInvalidOp ouch);

  /// Removes a directory.
  void rmdir(1:string path) throws (1:RpcInvalidOp ouch);

  /// Reads a directory.
  RpcFileList readdir(1:string path) throws (1:RpcInvalidOp ouch);

  void add_subfile(1:string parent, 2:string subfile)
	throws (1:RpcInvalidOp ouch);

  void remove_subfile(1:string parent, 2:string subfile)
    throws (1:RpcInvalidOp ouch);

  /// Creates a file and returns its object ID.
  RpcObjectId create(1:string path, 2:i64 mode, 3:i64 uid, 4:i64 gid)
    throws (1:RpcInvalidOp ouch);

  /// Obtains the object ID for a file.
  RpcObjectId object_id(1:string path) throws (1:RpcInvalidOp ouch);

  /// Find the objects IDs for given files.
  RpcObjectList find_objects(1:RpcFileList files) throws (1:RpcInvalidOp ouch);

  /// Removes a file.
  void remove(1:string path)
    throws (1:RpcInvalidOp ouch);

  /// Access the attributes of a file or a directory.
  RpcFileInfo getattr(1:string path)
    throws (1:RpcInvalidOp ouch);

  /// Returns a list of file paths.
  RpcFileList find_files(1:RpcObjectList objects) throws (1:RpcInvalidOp ouch);

  /**
   * \brief Creates an index and assign it to a IndexServer in the hash
   * ring.
   * \return the address of IndexServer to manages this index.
   */
  void create_index(1:RpcIndexCreateRequest index)
    throws (1:RpcInvalidOp ouch);

  /// Removes the related items of an index completely.
  void remove_index(1:string root, 2:string name) throws (1:RpcInvalidOp ouch);

  /**
   * \brief Returns the index paths for all index that have 'names', which also
   * includes files under 'root' path.
   * \return a list of index paths.
   */
  list<string> locate_indices_for_search(
	  1:string root, 2:list<string> names) throws (1:RpcInvalidOp ouch);

  /// Locate all indices under the directory.
  list<string> locate_indices(1:RpcIndexInfoRequest request)
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
  void create_index(1:RpcIndexCreateRequest index)
	  throws (1:RpcInvalidOp ouch);

  /**
   * \Remove an entire index.
   */
  void remove_index(1:RpcIndexName name) throws (1:RpcInvalidOp ouch);

  void update(1:RpcIndexUpdate updates) throws (1:RpcInvalidOp ouch);

  list<RpcObjectId> search(1:RpcComplexQuery query)
	throws (1:RpcInvalidOp ouch);

  /**
   * \brief Queries information for a single index partition.
   */
  RpcIndexInfo info(1:RpcIndexInfoRequest request)
	throws (1:RpcInvalidOp ouch);
}
