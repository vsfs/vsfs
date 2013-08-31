/*
 * Copyright 2011-2013 (c) Lei Xu <eddyxu@gmail.com>
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

#ifndef VSFS_FUSE_VSFS_OPS_H_
#define VSFS_FUSE_VSFS_OPS_H_

#include <boost/utility.hpp>
#include <fuse/fuse.h>
#include <sys/types.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>  // NOLINT
#include "vobla/macros.h"
#include "vsfs/common/file_object.h"
#include "vsfs/common/storage_manager.h"
#include "vsfs/client/vsfs_rpc_client.h"

#if FUSE_MAJOR_VERSION == 2 and FUSE_MINOR_VERSION >= 9
#define FUSE_29 1
#endif

using std::mutex;
using std::string;
using std::thread;
using std::unique_ptr;

// forward declaration
class Status;
struct stat;
struct timespec;
struct fuse_file_info;

namespace vsfs {
namespace fuse {

/**
 * \class VsfsFuse vsfs_ops.h "vsfs/fuse/vsfs_ops.h"
 * \brief Vsfs's FUSE implementation.
 */
class VsfsFuse : boost::noncopyable {
 public:
  /**
   * \brief Initialize all components to connect VSFS before mount the FUSE
   * layer.
   * \note Must call it before all fuse operations.
   *
   * \param basedir the basedir to mount the underlying FS.
   * \param host the host name for master node.
   * \param port the port of the master node.
   *
   * The order of FUSE initialization:
   *
   *  Status status = VsfsFuse::init(...);
   *  if (!status.ok()) {
   *     // error handling.
   *  }
   *  fuse_main(...);
   */
  static Status init(const string &basedir, const string &mount_point,
                     const string &host, int port);

  /// Destroy VsfsFuse object.
  static void destory();

  static VsfsFuse* instance();

  ~VsfsFuse();

  const string& basedir() const;

  string mnt_path(const string &vsfs_path) const;

  StorageManager* storage_manager() {
    return storage_manager_.get();
  }

  client::VSFSRpcClient* client() const {
    return client_.get();
  }

  void add_obj(uint64_t fd, FileObject* file_obj);

  Status close_obj(uint64_t fd);

  FileObject* get_obj(uint64_t fd);

 private:
  VsfsFuse(const string &basedir, const string &mount_point,
           const string &host, int port);

  string basedir_;

  string mount_point_;

  string host_;

  int port_;

  unique_ptr<StorageManager> storage_manager_;

  unique_ptr<client::VSFSRpcClient> client_;

  std::map<uint64_t, unique_ptr<FileObject>> fh_to_obj_map_;

  // The mutex to protect fh_to_obj_mpa_
  mutex obj_map_mutex_;
};

// VSFS operations
int vsfs_access(const char *path, int flag);
int vsfs_chmod(const char *path, mode_t mode);
int vsfs_chmod(const char *path, mode_t mode);
int vsfs_chown(const char *path, uid_t uid, gid_t gid);
int vsfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int vsfs_getattr(const char *path, struct stat *stbuf);
int vsfs_fgetattr(const char *, struct stat*, struct fuse_file_info*);
int vsfs_mkdir(const char *path, mode_t mode);
int vsfs_open(const char *path, struct fuse_file_info *fi);
int vsfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi);
int vsfs_opendir(const char *path, struct fuse_file_info* info);
int vsfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi);
int vsfs_releasedir(const char *, struct fuse_file_info *);
int vsfs_release(const char *path, struct fuse_file_info *fi);
int vsfs_readlink(const char *path, char *buf, size_t size);
int vsfs_rmdir(const char *path);
int vsfs_truncate(const char *path, off_t offset);
int vsfs_unlink(const char *path);
int vsfs_utimens(const char *path, const struct timespec tv[2]);
int vsfs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi);
int vsfs_lock(const char*, struct fuse_file_info*, int cmd, struct flock*);
void* vsfs_init(struct fuse_conn_info *conn);
void vsfs_destroy(void *data);
int vsfs_flush(const char *, struct fuse_file_info *);
int vsfs_fsync(const char *, int, struct fuse_file_info*);
int vsfs_statfs(const char *, struct statvfs *);
int vsfs_getxattr(const char *path, const char *name, char *value, size_t vlen);

#if defined(FUSE_29)
int vsfs_write_buf(const char *, struct fuse_bufvec *buf, off_t off,
                   struct fuse_file_info *);
int vsfs_flock(const char *, struct fuse_file_info *, int op);

#endif

}   // namespace fuse
}   // namespace vsfs

#endif  // VSFS_FUSE_VSFS_OPS_H_
