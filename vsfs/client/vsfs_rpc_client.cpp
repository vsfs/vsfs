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

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/transport/TTransportException.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/common/path_util.h"
#include "vsfs/common/types.h"
#include "vsfs/index/index_info.h"
#include "vsfs/rpc/thrift_utils.h"
#include "vsfs/rpc/vsfs_types.h"

using apache::thrift::transport::TTransportException;
using std::map;
using std::set;
using std::string;
using std::to_string;
using std::vector;
using vobla::Status;
using vobla::ThreadPool;
using vsfs::index::IndexInfo;
namespace fs = boost::filesystem;

/*
DEFINE_int32(vsfs_client_num_thread, 16, "Sets the number of thread one "
             "VSFS client can use.");

DEFINE_int32(vsfs_client_batch_size, 2048, "Sets the batch size of sending "
             "index update requests.");
             */
const int FLAGS_vsfs_client_num_thread = 16;
const int FLAGS_vsfs_client_batch_size = 2048;

const int kNumBackOffs = 3;

namespace vsfs {

using rpc::RpcClientFactory;

namespace client {

namespace {

RpcNodeAddress string_to_address(const string& addr_str) {
  RpcNodeAddress result;
  vector<string> fields;
  boost::split(fields, addr_str, boost::is_any_of(":"));
  CHECK_EQ(2u, fields.size());
  result.host = fields[0];
  result.port = stoi(fields[1]);
  return result;
}

string address_to_string(const RpcNodeAddress& addr) {
  return addr.host + ":" + to_string(addr.port);
}

}  // namespace

VSFSRpcClient::VSFSRpcClient(const string &host, int port)
    : master_client_factory_(new RpcClientFactory<MasterClientType>),
      index_client_factory_(new RpcClientFactory<IndexClientType>),
      host_(host), port_(port),
      thread_pool_(FLAGS_vsfs_client_num_thread) {
}

VSFSRpcClient::VSFSRpcClient(MasterClientFactory* master_factory,
                             IndexClientFactory* index_factory)
    : master_client_factory_(master_factory),
      index_client_factory_(index_factory) {
}

VSFSRpcClient::~VSFSRpcClient() {
  thread_pool_.close();
  thread_pool_.join();
  if (master_client_.get() != NULL) {
    disconnect();
  }
}

Status VSFSRpcClient::sync_master_server_map() {
  VLOG(0) << "Caches master server Consistent Hashing Ring.";
  RpcConsistentHashRing ring;
  try {
    master_client_->handler()->get_all_masters(ring);
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to get_all_masters(): " << e.what();
    return Status(e.getType(), e.what());
  }
  for (const auto& sep_and_address : ring) {
    NodeInfo node_info;
    node_info.address = sep_and_address.second;
    auto sep = sep_and_address.first;
    master_map_.add(sep, node_info);
  }
  return Status::OK;
}

Status VSFSRpcClient::sync_index_server_map() {
  VLOG(0) << "Caches index server Consistent Hashing Ring.";
  RpcConsistentHashRing ring;
  try {
    master_client_->handler()->get_all_index_servers(ring);
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to get_all_index_servers(): " << e.what();
    return Status(e.getType(), e.what());
  }
  for (const auto& sep_and_address : ring) {
    NodeInfo node_info;
    node_info.address = sep_and_address.second;
    auto sep = sep_and_address.first;
    index_server_map_.add(sep, node_info);
  }
  return Status::OK;
}

Status VSFSRpcClient::init() {
  VLOG(0) << "Initialize a VSFSRpcClient.";
  auto status = connect(host_, port_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to connect to primary master("
               << host_ << ":" << port_ << "), because: "
               << status.message();
    return status;
  }
  status = sync_master_server_map();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to synchronize local master server mapping with "
               << " the primary master server: " << host_ << ":" << port_
               << ", because: " << status.message();
    return status;
  }
  status = sync_index_server_map();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to synchronize local index server mapping with "
               << " the primary master server: " << host_ << ":" << port_
               << ", because: " << status.message();
    return status;
  }
  return Status::OK;
}

Status VSFSRpcClient::connect(const string &host, int port) {
  host_ = host;
  port_ = port;
  master_client_ = master_client_factory_->open(host, port);
  VLOG(1) << "Connecting to " << host_ << ":" << port;
  try {
    master_client_->open();
    VLOG(1) << "Successfully connected to masterd: " << host << ":" << port;
    return Status::OK;
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to connect to master node: "
        << host << ":" << port;
    return Status(e.getType(), e.what());
  } catch (...) {  // NOLINT
    LOG(ERROR) << "Unknown connection error. ";
    return Status(-1, "Unknown connection error");
  }
}

Status VSFSRpcClient::disconnect() {
  try {
    master_client_->close();
    return Status::OK;
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Failed to disconnect from the master node: "
        << host_ << ":" << port_;
    return Status(e.getType(), e.what());
  }
}

Status VSFSRpcClient::create(const string &path, int64_t mode, int64_t uid,
                             int64_t gid, ObjectId* id) {
  if (!is_initialized()) {
    VLOG(1) << "The VSFS RPC client has not initialized yet.";
    return Status(-1, "The client has not initialized yet.");
  }
  auto hash = PathUtil::path_to_hash(path);
  NodeInfo file_node;
  auto status = master_map_.get(hash, &file_node);
  if (!status.ok()) {
    VLOG(0) << "Failed to find a master server to create file: " << path;
    return status;
  }
  mode |= S_IFREG;
  // Number of retries to create file to solve the conflicts.
  int backoff = kNumBackOffs;
  while (backoff) {
    try {
      auto client = master_client_factory_->open(file_node.address.host,
                                                 file_node.address.port);
      *id = client->handler()->create(path, mode, uid, gid);
      master_client_factory_->close(client);
      break;
    } catch (TTransportException e) {  // NOLINT
      // TODO(eddyxu): clear the conflict resolving algorithm later.
      status.set_error(e.getType());
      status.set_message(e.what());
      backoff--;
      continue;
    }
  }
  if (!status.ok()) {
    VLOG(0) << "Failed to create file: "
            << path << " because: " << status.message();
    return status;
  }
  status = add_subfile(path);
  if (!status.ok()) {
    // Roll back the created file record from `file_node`.
    try {
      auto client = master_client_factory_->open(file_node.address);
      client->handler()->remove(path);
      master_client_factory_->close(client);
    } catch (TTransportException e) {  // NOLINT
      return Status(e.getType(), e.what());
    }
  }
  return Status::OK;
}

Status VSFSRpcClient::open(const string& path, ObjectId* oid) {
  CHECK_NOTNULL(oid);
  auto hash = PathUtil::path_to_hash(path);
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    auto client = master_client_factory_->open(node.address);
    *oid = client->handler()->object_id(path);
    master_client_factory_->close(client);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }
  return Status::OK;
}

Status VSFSRpcClient::unlink(const string& path, ObjectId* oid) {
  // First remove the subfile from its parent node.
  auto status = remove_subfile(path);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove " << path << " from namespace";
    return status;
  }

  NodeInfo node;
  CHECK(master_map_.get(path, &node).ok());
  try {
    auto client = master_client_factory_->open(node.address);
    *oid = client->handler()->remove(path);
    master_client_factory_->close(client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    return Status(ouch.what, ouch.why);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }
  return Status::OK;
}

Status VSFSRpcClient::mkdir(
    const string& path, int64_t mode, int64_t uid, int64_t gid) {
  if (!is_initialized()) {
    VLOG(1) << "The VSFS RPC client has not initialized yet.";
    return Status(-1, "The client has not initialized yet.");
  }
  HashValueType hash = PathUtil::path_to_hash(path);
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    auto master_client = master_client_factory_->open(node.address.host,
                                                      node.address.port);
    RpcFileInfo dir_info;
    dir_info.__set_mode(mode | S_IFDIR);
    dir_info.__set_uid(uid);
    dir_info.__set_gid(gid);
    master_client->handler()->mkdir(path, dir_info);
    master_client_factory_->close(master_client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    status = Status(ouch.what, ouch.why);
    return status;
  } catch (TTransportException e) {  // NOLINT
    status = Status(e.getType(), e.what());
    LOG(ERROR) << "Failed to run mkdir RPC to master node {"
               << node.address.host << ":" << node.address.port
               << "} : " << status.message();
    return status;
  }
  status = add_subfile(path);
  if (!status.ok()) {
    // Roll back.
    try {
      auto client = master_client_factory_->open(node.address);
      client->handler()->remove(path);
      master_client_factory_->close(client);
    } catch (TTransportException e) {  // NOLINT
      return Status(e.getType(), e.what());
    }
  }
  return Status::OK;
}

Status VSFSRpcClient::rmdir(const string& path) {
  // TODO(eddyxu): refactory mkdir/rmdir and extract shared code.
  if (!is_initialized()) {
    VLOG(1) << "The VSFS RPC client has not initialized yet.";
    return Status(-1, "The client has not initialized yet.");
  }
  auto status = remove_subfile(path);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove " << path << " from namespace";
    return status;
  }
  auto hash = PathUtil::path_to_hash(path);
  NodeInfo node;
  status = master_map_.get(hash, &node);
  if (!status.ok()) {
    return status;
  }
  try {
    auto master_client = master_client_factory_->open(node.address.host,
                                                      node.address.port);
    master_client->handler()->rmdir(path);
    master_client_factory_->close(master_client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    LOG(ERROR) << "Failed to rmdir: " << ouch.why;
    return Status(ouch.what, ouch.why);
  } catch (TTransportException e) {  // NOLINT
    status = Status(e.getType(), e.what());
    LOG(ERROR) << "Failed to run mkdir RPC to master node: "
        << node.address.host << ":" << node.address.port
        << " because: " << status.message();
    return status;
  }
  return Status::OK;
}

Status VSFSRpcClient::readdir(const string& dirpath, vector<string>* files) {  // NOLINT
  CHECK_NOTNULL(files);
  auto hash = PathUtil::path_to_hash(dirpath);
  NodeInfo node;
  CHECK(master_map_.get(hash, &node).ok());
  try {
    auto client = master_client_factory_->open(node.address);
    client->handler()->readdir(*files, dirpath);
    master_client_factory_->close(client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    return Status(ouch.what, ouch.why);
  } catch (TTransportException e) {  // NOLINT
    return Status(e.getType(), e.what());
  }
  return Status::OK;
}

Status VSFSRpcClient::getattr(const string& path, struct stat* stbuf) {
  CHECK_NOTNULL(stbuf);
  NodeInfo node;
  CHECK(master_map_.get(path, &node).ok());
  RpcFileInfo file_info;
  try {
    auto client = master_client_factory_->open(node.address);
    client->handler()->getattr(file_info, path);
    master_client_factory_->close(client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    return Status(ouch.what, ouch.why);
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Thrift Transport Exception: (" << e.getType() << "): "
               << e.what();
    return Status(e.getType(), e.what());
  }
  stbuf->st_mode = file_info.mode;
  stbuf->st_size = file_info.size;
  stbuf->st_uid = file_info.uid;
  stbuf->st_gid = file_info.gid;
  stbuf->st_atime = file_info.atime;
  stbuf->st_ctime = file_info.ctime;
  stbuf->st_mtime = file_info.mtime;
  return Status::OK;
}

Status VSFSRpcClient::setattr(const string& path, const RpcFileInfo& info) {
  NodeInfo node;
  CHECK(master_map_.get(path, &node).ok());
  Status status;
  try {
    auto client = master_client_factory_->open(node.address);
    client->handler()->setattr(path, info);
    master_client_factory_->close(client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    status.set(ouch.what, ouch.why);
  } catch (TTransportException e) {  // NOLINT
    LOG(ERROR) << "Thrift Transport Exception: (" << e.getType() << "): "
               << e.what();
    status.set(e.getType(), e.what());
  }
  return status;
}

Status VSFSRpcClient::chmod(const string& path, mode_t mode) {
  RpcFileInfo file_info;
  file_info.__set_mode(mode);
  return setattr(path, file_info);
}

Status VSFSRpcClient::chown(const string& path, int64_t uid, int64_t gid) {
  RpcFileInfo file_info;
  file_info.__set_uid(uid);
  file_info.__set_gid(gid);
  return setattr(path, file_info);
}

Status VSFSRpcClient::utimens(const string& path,
                              int64_t atime, int64_t mtime) {
  RpcFileInfo file_info;
  file_info.__set_atime(atime);
  file_info.__set_mtime(mtime);
  return setattr(path, file_info);
}

Status VSFSRpcClient::create_index(const string& root, const string& name,
                                   int index_type, int key_type,
                                   int64_t mode, int64_t uid, int64_t gid) {
  struct stat stbuf;
  auto status = getattr(root, &stbuf);
  if (!status.ok()) {
    LOG(ERROR) << "Can not create index: " << status.message();
    return status;
  }
  if (!S_ISDIR(stbuf.st_mode)) {
    LOG(ERROR) << "Can not create index on an non-dir path.";
    return Status::system_error(ENOTDIR);
  }

  string partition_path = PathUtil::partition_path(root, name, 0);
  RpcIndexCreateRequest create_request;
  create_request.root = partition_path;
  create_request.name = name;
  create_request.index_type = index_type;
  create_request.key_type = key_type;
  create_request.mode = mode;
  create_request.uid = uid;
  create_request.gid = gid;

  auto hash = PathUtil::path_to_hash(partition_path);
  NodeInfo index_server;
  status = index_server_map_.get(hash, &index_server);
  CHECK(status.ok());
  try {
    auto index_client = index_client_factory_->open(index_server.address);
    index_client->handler()->create_index(create_request);
    index_client_factory_->close(index_client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    status = Status(ouch.what, ouch.why);
  } catch (TTransportException e) {  // NOLINT
    status = Status(e.getType(), e.what());
  }
  if (!status.ok()) {
    LOG(ERROR) << "Failed to create index on IndexServer: "
               << index_server.address.host << ":" << index_server.address.port
               << " because " << status.message();
    return status;
  }

  // Recursively creates index directories.
  status = mkdir(root + "/.vsfs", mode, uid, gid);
  if (!status.ok() && status.error() != -EEXIST) {
    // Roll back
    status = remove_index(root, name);
    return status;
  }
  status = Status::OK;  // resets the status.

  auto full_path = PathUtil::index_path(root, name);
  hash = PathUtil::path_to_hash(full_path);
  NodeInfo master_server;
  CHECK(master_map_.get(hash, &master_server).ok());
  try {
    auto client = master_client_factory_->open(master_server.address);
    create_request.root = root;
    create_request.name = name;
    client->handler()->create_index(create_request);
    master_client_factory_->close(client);
  } catch (RpcInvalidOp ouch) {  // NOLINT
    status = Status(ouch.what, ouch.why);
  } catch (TTransportException e) {  // NOLINT
    status = Status(e.getType(), e.what());
  }
  if (!status.ok()) {
    // Roll back. Note that it does not delete '<root>/.vsfs' directory.
    LOG(ERROR) << "Failed to create index: " << root << ":" << name
               << " because: " << status.message()
               << "\nRolling back...";
    status = remove_index(root, name);
  }

  // TODO(lxu): should pass oid to index server to store this index.
  ObjectId oid;
  status = create(PathUtil::partition_path(root, name, 0),
                  0755, uid, gid, &oid);
  if (!status.ok()) {
    rmdir(PathUtil::index_path(root, name));
    remove_index(root, name);
  }
  return Status::OK;
}

Status VSFSRpcClient::remove_index(const string& root, const string& name) {
  RpcIndexName request;
  request.root = root;
  request.name = name;

  vector<string> partitions;
  string full_index_path = PathUtil::index_path(name, root);
  auto status = this->readdir(full_index_path, &partitions);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to get all partitions: " << status.message();
    return status;
  }
  // Tries the best-efforts to delete all partitions.
  for (const auto& part : partitions) {
    auto partition_path = (fs::path(full_index_path) / part).string();
    ObjectId obj_id;
    unlink(partition_path, &obj_id);
    auto hash = PathUtil::path_to_hash(partition_path);
    NodeInfo node;
    CHECK(index_server_map_.get(hash, &node).ok());
    try {
      auto index_client = index_client_factory_->open(node.address);
      index_client->handler()->remove_index(request);
      index_client_factory_->close(index_client);
    } catch (RpcInvalidOp ouch) {  // NOLINT
      status.set(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      LOG(ERROR) << "Failed to remove index from IndexServer: "
                 << node.address.host << ":" << node.address.port
                 << " : " << e.what();
      status.set(e.getType(), e.what());
    }
  }
  return status;
}

Status VSFSRpcClient::search(const ComplexQuery& query,
                             vector<string>* results) {
  CHECK_NOTNULL(results);
  SearchPlanMap plan;
  auto status = gen_search_plan(query, &plan);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to generate search plan: " << status.message();
    return status;
  }

  vector<ObjectId> objects;
  // TODO(lxu): use multithread to do parallel query.
  for (const auto& addr_and_partitions : plan) {
    auto address = string_to_address(addr_and_partitions.first);
    RpcComplexQuery rpc_query;
    rpc_query.root = query.root();
    for (const auto& part : addr_and_partitions.second) {
      auto index_name = fs::path(part).parent_path().filename().string();
      auto range = query.range_query(index_name);
      rpc_query.range_queries.emplace_back();
      auto& sub_query = rpc_query.range_queries.back();
      sub_query.index_path = part;
      sub_query.name = index_name;
      sub_query.upper = range->upper;
      sub_query.upper_open = !range->upper_closed;
      sub_query.lower = range->lower;
      sub_query.lower_open = !range->lower_closed;
    }
    vector<ObjectId> tmp;
    try {
      auto client = index_client_factory_->open(address);
      client->handler()->search(tmp, rpc_query);
      index_client_factory_->close(client);
      objects.insert(end(objects), begin(tmp), end(tmp));
    } catch (RpcInvalidOp ouch) {  // NOLINT
      status.set(ouch.what, ouch.why);
      break;
    } catch (TTransportException e) {  // NOLINT
      status.set(e.getType(), e.what());
      break;
    }
  }
  if (!status.ok()) {
    LOG(ERROR) << "VsfsRpcClient::search(): failed to search object ID: "
               << status.message();
    return status;
  }

  status = get_file_paths_from_objects(objects, results);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to resolve file path: " << status.message();
    return status;
  }
  return Status::OK;
}

VSFSRpcClient::IndexUpdateTask::IndexUpdateTask(VSFSRpcClient* parent)
    : parent_(parent) {
  CHECK_NOTNULL(parent);
}

void VSFSRpcClient::IndexUpdateTask::add(const IndexUpdateRequest* request) {
  CHECK_NOTNULL(request);
  requests_.emplace_back(request);
}

size_t VSFSRpcClient::IndexUpdateTask::size() const {
  return requests_.size();
}

Status VSFSRpcClient::IndexUpdateTask::get_parent_path_to_index_path_map(
    ParentPathToIndexPathMap *index_map) {
  CHECK_NOTNULL(index_map);
  // map<parent path, vector<index names>>
  map<string, set<string>> parent_to_index_names_map;
  for (const auto request : requests_) {
    auto parent = fs::path(request->file_path).parent_path().string();
    parent_to_index_names_map[parent].insert(request->index_name);
  }

  // map<parent path, map<index name, actual index path>>
  for (const auto& parent_and_names : parent_to_index_names_map) {
    auto parent = parent_and_names.first;
    for (const auto& index_name : parent_and_names.second) {
      auto tmp_parent = parent;
      while (true) {
        auto index_path = PathUtil::index_path(tmp_parent, index_name);
        struct stat stbuf;
        auto status = parent_->getattr(index_path, &stbuf);
        if (status.ok() && S_ISDIR(stbuf.st_mode)) {
          (*index_map)[parent][index_name] = index_path;
          break;
        }
        if (!status.ok() && status.error() != -ENOENT) {
          LOG(ERROR) << "Failed to reorder requests because can not check the "
              << "existence of index: " << status.message();
          return status;
        }
        if (tmp_parent == "/") {
          LOG(ERROR) << "Failed to find a valid index for index name: "
                     << index_name;
          return Status(-1, "Failed to find valid index.");
        }
        tmp_parent = fs::path(tmp_parent).parent_path().string();
      }
    }
  }

  return Status::OK;
}

Status VSFSRpcClient::IndexUpdateTask::reorder_requests_to_index_servers(
    const ParentPathToIndexPathMap& index_map,
    ServerToRequestMap* request_map) {
  CHECK_NOTNULL(request_map);

  for (size_t pos = 0; pos < requests_.size(); ++pos) {
    const auto request = requests_[pos];
    // The index path must exist, otherwise an exception will raise.
    string parent = fs::path(request->file_path).parent_path().string();
    const auto& index_path = index_map.at(parent)
                                      .at(request->index_name);
    // TODO(lxu): the current solution only works for one partition.
    auto partition_path = index_path + "/0";
    auto hash = PathUtil::path_to_hash(partition_path);
    NodeInfo node;
    CHECK(parent_->index_server_map_.get(hash, &node).ok());
    string addr = node.address.host + ":" + to_string(node.address.port);
    (*request_map)[addr][partition_path].push_back(pos);
  }
  return Status::OK;
}

Status VSFSRpcClient::IndexUpdateTask::run() {
  VLOG(1) << "Find the index path for each request.";
  ParentPathToIndexPathMap index_map;
  auto status = get_parent_path_to_index_path_map(&index_map);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to get index path for each request: "
               << status.message();
    return status;
  }

  ServerToRequestMap request_map;
  status = reorder_requests_to_index_servers(index_map, &request_map);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to reorder IndexUpdateRequest: " << status.message();
    return status;
  }

  vector<ObjectId> object_ids;
  vector<string> paths;
  for (const auto& req : requests_) {
    paths.push_back(req->file_path);
  }
  status = parent_->find_objects(paths, &object_ids);
  if (!status.ok()) {
    LOG(ERROR) << "IndexUpdateTask::run(): failed to resolve object IDs."
               << status.message();
    return status;
  }

  for (const auto& server_and_requests : request_map) {
    RpcIndexUpdate update_request;
    // TODO(lxu): clean up the confusion names in Rpc structurs.
    for (const auto& index_path_and_pos : server_and_requests.second) {
      update_request.updates.emplace_back();
      auto& record_list = update_request.updates.back();
      record_list.root_path = index_path_and_pos.first;
      for (const auto& pos : index_path_and_pos.second) {
        auto req = requests_[pos];
        record_list.record_updates.emplace_back();
        auto& record = record_list.record_updates.back();
        // TODO(lxu): merges IndexUpdateRequest::OP with RpcIndexUpdateOpCode.
        switch (req->op) {
          case IndexUpdateRequest::INSERT:
            record.op = RpcIndexUpdateOpCode::INSERT;
            break;
          case IndexUpdateRequest::UPDATE:
            record.op = RpcIndexUpdateOpCode::UPDATE;
            break;
          case IndexUpdateRequest::REMOVE:
            record.op = RpcIndexUpdateOpCode::REMOVE;
            break;
          case IndexUpdateRequest::UNKNOWN:
          default:
            record.op = RpcIndexUpdateOpCode::UNKNOWN;
            break;
        }
        record.key = req->key;
        record.__set_value(to_string(object_ids[pos]));
        VLOG(3) << "Index update record: key:" << req->key
                << " value: " << req->file_path << "(OBJ:" << record.value
                << ") root: " << record_list.root_path
                << " name: " << record_list.name;
      }
    }

    auto address = string_to_address(server_and_requests.first);
    try {
      auto index_conn = parent_->index_client_factory_->open(address);
      index_conn->handler()->update(update_request);
      parent_->index_client_factory_->close(index_conn);
    } catch (RpcInvalidOp ouch) {  // NOLINT
      return Status(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      LOG(ERROR) << "Thrift transport exception: " << e.what();
      return Status(e.getType(), e.what());
    }
  }
  return Status::OK;
}

Status VSFSRpcClient::update(const vector<IndexUpdateRequest>& requests) {
  vector<unique_ptr<IndexUpdateTask>> tasks;
  tasks.emplace_back(unique_ptr<IndexUpdateTask>(new IndexUpdateTask(this)));
  vector<ThreadPool::FutureType> futures;
  size_t tasks_in_pool = 0;
  for (const auto& request : requests) {
    tasks.back()->add(&request);
    if (tasks.back()->size() >
        static_cast<size_t>(FLAGS_vsfs_client_batch_size)) {
      futures.emplace_back(
          thread_pool_.add_task(
              std::bind(&IndexUpdateTask::run, tasks.back().get())));
      tasks.emplace_back(unique_ptr<IndexUpdateTask>(
              new IndexUpdateTask(this)));
    }
  }
  if (tasks_in_pool < tasks.size()) {
    futures.emplace_back(
        thread_pool_.add_task(std::bind(&IndexUpdateTask::run,
                                        tasks.back().get())));
  }
  for (auto& future : futures) {
    if (!future.get().ok()) {
      return future.get();
    }
  }
  return Status::OK;
}

Status VSFSRpcClient::info(const string& path,
                           vector<index::IndexInfo>* infos) {
  CHECK_NOTNULL(infos);
  RpcIndexInfoRequest request;
  request.path = path;
  request.__set_recursive(true);

  auto master_map = master_map_.get_ch_ring_as_map();
  vector<string> indices;
  Status status;
  for (const auto& hash_and_node : master_map) {
    const auto& node = hash_and_node.second;
    vector<string> tmp;
    try {
      auto client = master_client_factory_->open(node.address);
      client->handler()->locate_indices(tmp, request);
      master_client_factory_->close(client);
    } catch (RpcInvalidOp ouch) {  // NOLINT
      status.set(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      status.set(e.getType(), e.what());
    }
    if (!status.ok()) {
      LOG(ERROR) << "Failed to locate indices on MasterNode: "
          << node.address.host << ":" << node.address.port
          << " because: " << status.message();
      return status;
    }
    indices.insert(indices.end(), begin(tmp), end(tmp));
  }

  for (const auto& idx_path : indices) {
    string root, name;
    if (!PathUtil::split_index_path(idx_path, &root, &name)) {
      LOG(ERROR) << "Index Path has wrong format: " << idx_path;
      return Status(-1, "Wrong index path format.");
    }

    auto partition_path = idx_path + "/0";
    NodeInfo node;
    CHECK(index_server_map_.get(partition_path, &node).ok());
    RpcIndexInfo rpc_info;
    RpcIndexInfoRequest req;
    req.path = partition_path;
    VLOG(1) << "Getting Index Info for " << idx_path;
    try {
      auto client = index_client_factory_->open(node.address);
      client->handler()->info(rpc_info, req);
      index_client_factory_->close(client);
    } catch (RpcInvalidOp ouch) {  // NOLINT
      status.set(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      status.set(e.getType(), e.what());
    }
    if (!status.ok()) {
      LOG(ERROR) << "Can not access the index info for: " << idx_path
                 << ", because: " << status.message();
      return status;
    }
    VLOG(1) << "IndexInfo: " << root << " " << name
            << " " << rpc_info.type << " " << rpc_info.key_type;
    infos->emplace_back(root, name, rpc_info.type, rpc_info.key_type);
  }

  return Status::OK;
}

bool VSFSRpcClient::is_initialized() {
  return !master_map_.empty();
}

Status VSFSRpcClient::find_objects(const vector<string>& paths,
                                   vector<ObjectId>* objects) {
  CHECK_NOTNULL(objects);
  objects->reserve(paths.size());

  map<string, vector<size_t>> reordered_path_pos_map;
  for (size_t i = 0; i < paths.size(); ++i) {
    NodeInfo node;
    CHECK(master_map_.get(paths[i], &node).ok());
    auto addr = address_to_string(node.address);
    reordered_path_pos_map[addr].push_back(i);
  }

  for (const auto& addr_and_pos : reordered_path_pos_map) {
    vector<string> tmp_files;
    vector<ObjectId> tmp_objs;
    const auto& positions = addr_and_pos.second;
    for (auto pos : positions) {
      tmp_files.push_back(paths[pos]);
    }
    RpcNodeAddress addr = string_to_address(addr_and_pos.first);
    try {
      auto client = master_client_factory_->open(addr);
      client->handler()->find_objects(tmp_objs, tmp_files);
      master_client_factory_->close(client);
    } catch (RpcInvalidOp ouch) {  // NOLINT
      return Status(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      return Status(e.getType(), e.what());
    }
    CHECK_EQ(tmp_objs.size(), tmp_files.size());
    size_t num_files = tmp_files.size();
    for (size_t i = 0; i < num_files; ++i) {
      (*objects)[positions[i]] = tmp_objs[i];
    }
  }
  return Status::OK;
}


Status VSFSRpcClient::add_subfile(const string& path) {
  if (path == "/") {
    return Status::OK;
  }
  auto parent = fs::path(path).parent_path().string();
  auto hash = PathUtil::path_to_hash(parent);
  auto filename = fs::path(path).filename().string();
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  CHECK(status.ok());  // master_map_ must exist at this point.
  int backoff = kNumBackOffs;
  while (backoff) {
    try {
      auto client = master_client_factory_->open(node.address);
      client->handler()->add_subfile(parent, filename);
      master_client_factory_->close(client);
      break;
    } catch (RpcInvalidOp ouch) {  // NOLINT
      status.set(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      // TODO(eddyxu): clear the conflict resolving algorithm later.
      status.set(e.getType(), e.what());
      backoff--;
    }
  }
  return status;
}

Status VSFSRpcClient::remove_subfile(const string& path) {
  if (path == "/") {
    return Status::OK;
  }
  auto parent = fs::path(path).parent_path().string();
  auto hash = PathUtil::path_to_hash(parent);
  auto filename = fs::path(path).filename().string();
  NodeInfo node;
  auto status = master_map_.get(hash, &node);
  CHECK(status.ok());  // master_map_ must exist at this point.
  int backoff = kNumBackOffs;
  while (backoff) {
    try {
      auto client = master_client_factory_->open(node.address);
      client->handler()->remove_subfile(parent, filename);
      master_client_factory_->close(client);
      break;
    } catch (RpcInvalidOp ouch) {  // NOLINT
      status.set(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      status.set(e.getType(), e.what());
      backoff--;
    }
  }
  return status;
}

Status VSFSRpcClient::locate_index_for_search(const ComplexQuery& query,
                                              vector<string>* indices) {
  CHECK_NOTNULL(indices);

  auto root = query.root();
  auto names = query.get_names_of_range_queries();
  // TODO(lxu): use iterator to avoid data copy.
  auto master_map = master_map_.get_ch_ring_as_map();

  Status status;
  // TODO(lxu): use threads to do parallel query.
  for (const auto& hash_and_node : master_map) {
    const auto& node = hash_and_node.second;
    vector<string> results;
    try {
      auto client = master_client_factory_->open(node.address);
      client->handler()->locate_indices_for_search(results, root, names);
      master_client_factory_->close(client);
    } catch (RpcInvalidOp ouch) {  // NOLINT
      status.set(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      status.set(e.getType(), e.what());
    }
    if (!status.ok()) {
      LOG(ERROR) << "Failed to locate indices on MasterNode: "
                 << node.address.host << ":" << node.address.port
                 << " because: " << status.message();
      return status;
    }
    indices->insert(end(*indices), begin(results), end(results));
  }
  return Status::OK;
}

Status VSFSRpcClient::gen_search_plan(const ComplexQuery& query,
                                      SearchPlanMap* plan) {
  CHECK_NOTNULL(plan);
  vector<string> indices;
  auto status = locate_index_for_search(query, &indices);
  if (!status.ok()) {
    return status;
  }
  for (const auto& index_path : indices) {
    // TODO(lxu): only support 1 partition now.
    auto partition_path = index_path + "/0";
    auto hash = PathUtil::path_to_hash(partition_path);
    NodeInfo node;
    CHECK(index_server_map_.get(hash, &node).ok());
    auto addr = node.address.host + ":" + to_string(node.address.port);
    (*plan)[addr].insert(partition_path);
  }
  return Status::OK;
}

Status VSFSRpcClient::get_file_paths_from_objects(
    const vector<ObjectId>& objects, vector<string>* paths) {
  CHECK_NOTNULL(paths);
  map<string, RpcObjectList> plan;
  for (auto obj : objects) {
    NodeInfo node;
    CHECK(master_map_.get(obj, &node).ok());
    auto addr = address_to_string(node.address);
    plan[addr].push_back(obj);
  }
  // TODO(lxu): use threads to do parallel query.
  for (const auto& addr_and_obj_list : plan) {
    auto address = string_to_address(addr_and_obj_list.first);
    vector<string> tmp;
    try {
      auto client = master_client_factory_->open(address);
      client->handler()->find_files(tmp, addr_and_obj_list.second);
      master_client_factory_->close(client);
    } catch (RpcInvalidOp ouch) {  // NOLINT
      return Status(ouch.what, ouch.why);
    } catch (TTransportException e) {  // NOLINT
      return Status(e.getType(), e.what());
    }
    paths->insert(paths->end(), tmp.begin(), tmp.end());
  }
  return Status::OK;
}

}  // namespace client
}  // namespace vsfs
