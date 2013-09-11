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

#define FUSE_USE_VERSION 28
#include <boost/filesystem.hpp>
#include <dirent.h>
#include <fcntl.h>
#include <fuse.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#if FUSE_MAJOR_VERSION == 2 and FUSE_MINOR_VERSION == 9
#include <sys/file.h>
#endif
#include <attr/xattr.h>  // It must be after sys/types.h.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/common/file_object.h"
#include "vsfs/common/object_storage_manager.h"
#include "vsfs/common/posix_path.h"
#include "vsfs/common/posix_storage_manager.h"
#include "vsfs/common/thread.h"
#include "vsfs/common/types.h"
#include "vsfs/fuse/vsfs_ops.h"

using std::string;
using std::unique_ptr;
using std::vector;
using vobla::Status;
using vsfs::client::VSFSRpcClient;

namespace fs = boost::filesystem;

DEFINE_int32(object_storage_dirwidth, 8192,
             "Directory width for ObjectStorageManager");

namespace vsfs {
namespace fuse {

unique_ptr<VsfsFuse> instance_;
VsfsFuse *vsfs;

Status VsfsFuse::init(const string &basedir, const string &mnt,
                      const string &host, int port, const string& sm) {
  LOG(INFO) << "VsfsFuse starts initilizing...";
  // This function should be only called once.
  CHECK(instance_.get() == NULL)
      << "VsfsFuse should only be initialized once.";
  string absolute_basedir = fs::absolute(basedir).string();
  string absolute_mnt = fs::absolute(mnt).string();
  instance_.reset(new VsfsFuse(absolute_basedir, absolute_mnt, host, port, sm));
  auto status = instance_->storage_manager()->init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize StorageManager: " << status.message();
    return status;
  }
  vsfs = instance_.get();
  LOG(INFO) << "VsfsFuse fully initialized.";
  status = vsfs->client_->init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize RPC connection to VSFS: "
               << status.message();
    return status;
  }
  vsfs->client_->mkdir("/", 0777, fuse_get_context()->uid,
                       fuse_get_context()->gid);
  return Status::OK;
}

void VsfsFuse::destory() {
  LOG(INFO) << "VsfsFuse is shutting down...";
  instance_.reset();
}

VsfsFuse* VsfsFuse::instance() {
  return instance_.get();
}

VsfsFuse::VsfsFuse(const string &basedir, const string &mnt,
                   const string &host, int port, const string& sm)
  : basedir_(basedir), mount_point_(mnt), host_(host), port_(port),
    client_(new VSFSRpcClient(host, port)) {
  if (sm == "posix") {
    storage_manager_.reset(new PosixStorageManager(basedir));
  } else if (sm == "object") {
    storage_manager_.reset(
        new ObjectStorageManager(basedir, FLAGS_object_storage_dirwidth));
  } else {
    CHECK(false) << "Unknown StorageManager type: " << sm;
  }
}

VsfsFuse::~VsfsFuse() {
}

const string& VsfsFuse::basedir() const {
  return basedir_;
}

string VsfsFuse::mnt_path(const string &vsfs_path) const {
  return (fs::path(mount_point_) / vsfs_path).string();
}

void VsfsFuse::add_obj(uint64_t fd, FileObject* file_obj) {
  MutexGuard guard(obj_map_mutex_);
  fh_to_obj_map_[fd].reset(file_obj);
}

Status VsfsFuse::close_obj(uint64_t fd) {
  MutexGuard guard(obj_map_mutex_);
  auto it = fh_to_obj_map_.find(fd);
  CHECK(it != fh_to_obj_map_.end());
  auto file_obj = it->second.get();
  auto status = file_obj->close();
  fh_to_obj_map_.erase(fd);
  return status;
}

FileObject* VsfsFuse::get_obj(uint64_t fd) {
  MutexGuard guard(obj_map_mutex_);
  auto it = fh_to_obj_map_.find(fd);
  if (it == fh_to_obj_map_.end()) {
    return nullptr;
  } else {
    return it->second.get();
  }
}

// VSFS Operations

void* vsfs_init(struct fuse_conn_info *conn) {
  LOG(INFO) << "Fuse component initializing...";
  (void) conn;
  return nullptr;
}

void vsfs_destroy(void *data) {
  (void) data;
  VsfsFuse::destory();
}

int vsfs_statfs(const char*, struct statvfs *stbuf) {
  auto status = VsfsFuse::instance()->storage_manager()->statfs(stbuf);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to statvfs: " << status.message();
    return status.error();
  }
  return 0;
}

int vsfs_access(const char* path, int flag) {
  PosixPath vsp(path);
  if (!vsp.is_validate()) {
    return -EINVAL;
  } else if (!vsp.is_query()) {
    (void) flag;
    /// TODO(eddyxu): always success for now.
    return 0;
    // return access(abspath.c_str(), flag);
  } else if (vsp.is_query()) {
    assert(true);
  } else {
    return -ENOENT;
  }
  return 0;
}

int vsfs_getattr(const char* path, struct stat* stbuf) {
  PosixPath vsp(path);
  if (!vsp.is_validate()) {
    return -EINVAL;
  }
  if (vsp.is_query()) {
    stbuf->st_mode |= S_IFDIR;
  } else if (vsp.is_result()) {
    stbuf->st_mode |= S_IFLNK;
    stbuf->st_size = vsp.result().size();
  } else {
    // TODO(eddyxu): replace with the below commented code when the writes can
    // update file size.
    RpcFileInfo file_info;
    auto status = VsfsFuse::instance()->client()->getattr(path, &file_info);
    if (!status.ok()) {
      LOG(ERROR) << "GETATTR: Failed to getattr from master server: "
                 << status.message();
      return status.error();
    }
    if (S_ISDIR(file_info.mode)) {
      stbuf->st_uid = file_info.uid;
      stbuf->st_gid = file_info.gid;
      stbuf->st_mode = file_info.mode;
      stbuf->st_size = file_info.size;
      stbuf->st_atime = file_info.atime;
      stbuf->st_ctime = file_info.ctime;
      stbuf->st_mtime = file_info.mtime;
      return 0;
    }
    status = VsfsFuse::instance()->storage_manager()
        ->getattr(path, file_info.object_id, stbuf);
    if (!status.ok()) {
      return status.error();
    }
  }
  return 0;
}

int vsfs_fgetattr(const char*, struct stat* stbuf, struct fuse_file_info* fi) {
  int ret = fstat(fi->fh, stbuf);
  if (ret == -1) {
    return -errno;
  }
  return 0;
}

int vsfs_ioctl(const char*, int cmd, void *arg, struct fuse_file_info* fi,
               unsigned int flags, void *data) {
  (void) flags;
  (void) data;
  int ret = ioctl(fi->fh, cmd, arg);
  if (ret == -1) {
    return -errno;
  }
  return 0;
}

int vsfs_utimens(const char* path, const struct timespec tv[2]) {
  return VsfsFuse::instance()->client()
      ->utimens(path, tv[0].tv_sec, tv[1].tv_sec).error();
}

int vsfs_chmod(const char* path, mode_t mode) {
  return VsfsFuse::instance()->client()->chmod(path, mode).error();
}

int vsfs_chown(const char* path, uid_t uid, gid_t gid) {
  return VsfsFuse::instance()->client()->chown(path, uid, gid).error();
}

int vsfs_truncate(const char*, off_t) {
  return 0;
}

int vsfs_opendir(const char*, struct fuse_file_info*) {
  // TODO(lxu): check permission.
  return 0;
}

int vsfs_releasedir(const char* path, struct fuse_file_info* info) {
  (void) path;
  (void) info;
  return 0;
}

int vsfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                 off_t, struct fuse_file_info*) {
  VLOG(0) << "Readdir: " << path;
  PosixPath vsp(path);

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  Status status;
  if (vsp.is_query()) {
    ComplexQuery complex_query;
    status = complex_query.parse(path);
    vector<string> result_files;
    Status status = vsfs->client()->search(complex_query, &result_files);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to search: " << status.message();
      return status.error();
    }
    LOG(INFO) << result_files.size() << " files are found.";
    for (const string &file : result_files) {
      struct stat stbuf;
      status = vsfs->client()->getattr(file, &stbuf);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to get attribute for file: " << file
                   << status.error();
        return status.error();
      }
      stbuf.st_mode |= S_IFLNK;
      string name(file.size(), 0);
      std::transform(file.begin(), file.end(), name.begin(),
                     [](int c) { return c == '/' ? '#' : c; });
      filler(buf, name.c_str(), &stbuf, 0);
    }
  } else {
    vector<string> subfiles;
    status = VsfsFuse::instance()->client()->readdir(path, &subfiles);
    if (!status.ok()) {
      return status.error();
    }
    for (const auto& subfile : subfiles) {
      filler(buf, subfile.c_str(), NULL, 0);
    }
  }
  return 0;
}

/**
 * \brief Create a directory
 */
int vsfs_mkdir(const char* path, mode_t mode) {
  auto status = VsfsFuse::instance()->client()
      ->mkdir(path, mode, fuse_get_context()->uid, fuse_get_context()->gid);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to mkdir: " << status.message();
    return status.error();
  }
  status = VsfsFuse::instance()->storage_manager()->mkdir(path, mode);
  return status.error();
}

int vsfs_rmdir(const char* path) {
  auto status = VsfsFuse::instance()->client()->rmdir(path);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to rmdir: " << path << ": " << status.message();
    return status.error();
  }
  status = VsfsFuse::instance()->storage_manager()->rmdir(path);
  return status.error();
}

int vsfs_create(const char* path, mode_t mode, struct fuse_file_info *fi) {
  ObjectId oid;
  Status status = vsfs->client()->create(path, mode, getuid(), getgid(), &oid);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to create file: " << status.message();
    return status.error();
  }
  FileObject *file_obj;
  status = VsfsFuse::instance()->storage_manager()
      ->open(path, oid, fi->flags | O_CREAT, mode, &file_obj);
  if (!status.ok()) {
    LOG(ERROR) << "StorageManager failed to create file: " << status.message();
    return status.error();
  }
  int fd = file_obj->fd();
  fi->fh = fd;
  VsfsFuse::instance()->add_obj(fd, file_obj);
  return 0;
}

int vsfs_open(const char* path, struct fuse_file_info* fi) {
  ObjectId oid;
  auto status = vsfs->client()->open(path, &oid);
  if (!status.ok()) {
    LOG(ERROR) << "StorageManager failed to open file: " << status.message();
    return status.error();
  }
  FileObject *file_obj;
  status = VsfsFuse::instance()->storage_manager()
      ->open(path, oid, fi->flags, &file_obj);
  if (!status.ok()) {
    LOG(ERROR) << "StorageManager failed to open file: " << status.message();
    return status.error();
  }
  int fd = file_obj->fd();
  VsfsFuse::instance()->add_obj(fd, file_obj);
  fi->fh = fd;
  return 0;
}

int vsfs_unlink(const char* path) {
  ObjectId obj_id;
  Status status = vsfs->client()->unlink(path, &obj_id);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to remove a file from master server:"
               << status.message();
    return status.error();
  }
  status = VsfsFuse::instance()->storage_manager()->unlink(path, obj_id);
  return status.error();
}

int vsfs_release(const char* path, struct fuse_file_info *fi) {
  auto status = VsfsFuse::instance()->close_obj(fi->fh);
  if (!status.ok()) {
    LOG(ERROR) << "Closing fd=" << fi->fh;
    LOG(ERROR) << "Failed to release file: " << path << ": "
               << status.message();
  }
  return status.error();
}

int vsfs_readlink(const char* path, char* buf, size_t size) {
  PosixPath vsp(path);
  if (vsp.is_query()) {
  } else if (vsp.is_result()) {
    VLOG(1) << "Resolve query result: " << path;
    string result_file = vsp.result();
    string linked_file(result_file.size(), 0);
    std::transform(result_file.begin(), result_file.end(), linked_file.begin(),
                   [](int c) { return c == '#' ? '/' : c; });
    string mnt_path = VsfsFuse::instance()->mnt_path(linked_file);
    strncpy(buf, mnt_path.c_str(), size);
    buf[mnt_path.size()] = 0;
  } else {
    ObjectId object_id;
    auto status = VsfsFuse::instance()->client()->object_id(path, &object_id);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to get object ID for path: " << path;
      return status.error();
    }
    ssize_t retlen;
    status = VsfsFuse::instance()->storage_manager()
        ->readlink(path, object_id, buf, size, &retlen);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to read link: " << path << ": " << status.message();
      return status.error();
    }
  }
  return 0;
}

int vsfs_read(const char*, char *buf, size_t size, off_t offset,
              struct fuse_file_info* fi) {
  FileObject *file_obj = VsfsFuse::instance()->get_obj(fi->fh);
  if (!file_obj) {
    LOG(ERROR) << "File does not existed.";
    return -EBADF;
  }
  ssize_t nread = pread(fi->fh, buf, size, offset);
  if (nread == -1) {
    LOG(ERROR) << "VSFS_READ ERROR: " <<  strerror(errno);
    return -errno;
  }
  return nread;
}

int vsfs_write(const char*, const char* buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
  FileObject *file_obj = VsfsFuse::instance()->get_obj(fi->fh);
  if (!file_obj) {
    LOG(ERROR) << "File object does not exist.";
    return -EBADF;
  }
  ssize_t nwrite = file_obj->write(buf, size, offset);
  if (nwrite == -1) {
    LOG(ERROR) << strerror(errno);
    return -errno;
  }
  // TODO(lxu): need to update size in masterd.
  return nwrite;
}

int vsfs_flush(const char* , struct fuse_file_info* fi) {
  // We are using open(2), there is nothing to flush.
  int res;
  res = close(dup(fi->fh));
  if (res == -1) {
    return -errno;
  }
  return 0;
}

int vsfs_getxattr(const char* path, const char* name, char *value,
                  size_t vlen) {
  (void) path;
  (void) name;
  (void) value;
  (void) vlen;
  return 0;
}

#if defined(FUSE_29)

int vsfs_flock(const char* path, struct fuse_file_info *fi, int op) {
  if (flock(fi->fh, op) == -1) {
    LOG(ERROR) << "Failed to flock(" << path << ", " << op << "), fd="
               << fi->fh;
    return -errno;
  }
  return 0;
}

int vsfs_write_buf(const char* , struct fuse_bufvec *buf, off_t off,
                   struct fuse_file_info *fi) {
  ssize_t nwrite = 0;
  ssize_t total_write = 0;
  FileObject *file_obj = VsfsFuse::instance()->get_obj(fi->fh);
  if (!file_obj) {
    return -EINVAL;
  }
  for (size_t i = 0; i < buf->count; i++) {
    nwrite = file_obj->write(buf->buf[i].mem, buf->buf[i].size, off);
    if (nwrite == -1) {
      return -errno;
    }
    total_write += nwrite;
  }
  return total_write;
}

#endif

int vsfs_lock(const char*, struct fuse_file_info* fi, int cmd,
              struct flock* flock) {
  if (fcntl(fi->fh, cmd, flock) == -1) {
    return -errno;
  }
  return 0;
}

int vsfs_fsync(const char*, int, struct fuse_file_info* fi) {
  if (fsync(fi->fh) == 0) {
    return 0;
  }
  return -errno;
}

int vsfs_symlink(const char *path1, const char *path2) {
  int ret = symlink(path1, path2);
  if (ret == -1) {
    return -errno;
  }
  return 0;
}

}  // namespace fuse
}  // namespace vsfs
