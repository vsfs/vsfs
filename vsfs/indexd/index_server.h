/**
 * \file index_server_handler.h
 *
 * \brief RPC handler for IndexServer.
 *
 * Copyright 2012 (c) Lei Xu <eddyxu@gmail.com>
 */

#ifndef VSFS_INDEXD_INDEX_SERVER_H_
#define VSFS_INDEXD_INDEX_SERVER_H_

#include <string>
#include <vector>
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/vsfs_types.h"

using std::string;
using std::vector;

namespace vsfs {
namespace indexd {

class IndexController;

/**
 * \brief RPC handler for IndexServer.
 */
class IndexServer : public IndexServerIf {
 public:
  explicit IndexServer(IndexController *server);

  virtual ~IndexServer();

  virtual void create_index(const RpcIndexCreateRequest &index);

  virtual void remove_index(const RpcIndexName &name);

  virtual void update(const RpcIndexUpdate &updates);

  virtual void search(vector<int64_t>& results,  // NOLINT
                      const RpcComplexQuery& query);

  virtual void info(RpcIndexInfo &info, const RpcIndexInfoRequest &request);

  /* virtual void migrate(const RpcIndexMigrationData &data);

  virtual void update_to_remote_log(const RpcIndexUpdate& updates);

  virtual void join_taker_node_server();

  virtual void join_taker_node_index_partition(const RpcIndexInfo& idx_info,
                                               int64_t sep); */

 private:
  IndexController *server_;
};

}  // namespace indexd
}  // namespace vsfs
#endif  // VSFS_INDEXD_INDEX_SERVER_H_
