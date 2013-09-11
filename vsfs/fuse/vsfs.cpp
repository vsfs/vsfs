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

#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <fuse_opt.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stddef.h>
#include <string.h>
#include <cstdio>
#include <string>
#include "vobla/status.h"
#include "vsfs/fuse/vsfs_ops.h"

using std::string;
using namespace vsfs::fuse;  // NOLINT

DECLARE_int32(object_storage_dirwidth);

/** command line options */
struct options {
  char *basedir;
  char *master_host;
  char *storage;  // Name of the storage manager.
  int dirwidth;
  int master_port;
} options;

#define VSFS_OPT_KEY(t, p, v) { t, offsetof(struct options, p), v }

enum {
  KEY_DEBUG,
  KEY_HELP,
  KEY_TRACK_ACCESS,
  KEY_VERSION,
};

struct fuse_opt vsfs_opts[] = {
  VSFS_OPT_KEY("--basedir %s", basedir, 0),
  VSFS_OPT_KEY("-b %s", basedir, 0),
  VSFS_OPT_KEY("--host %s", master_host, 0),
  VSFS_OPT_KEY("-H %s", master_host, 0),
  VSFS_OPT_KEY("--port %d", master_port, 9876),
  VSFS_OPT_KEY("-p %d", master_port, 9876),
  VSFS_OPT_KEY("--storage %s", storage, 0),
  VSFS_OPT_KEY("-s %s", storage, 0),
  VSFS_OPT_KEY("-w %d", dirwidth, 8192),
  VSFS_OPT_KEY("--dirwidth %d", dirwidth, 8192),

  FUSE_OPT_KEY("--version", KEY_VERSION),
  FUSE_OPT_KEY("-h", KEY_HELP),
  FUSE_OPT_KEY("--help", KEY_HELP),
  FUSE_OPT_KEY("--debug", KEY_DEBUG),
  // note: must change FUSE_OPT_END to {NULL, 0, 0} to pass g++ compiling
  {NULL, 0, 0}
};

int vsfs_opt_proc(void *, const char *, int key, struct fuse_args *outargs) {
  static fuse_operations opers;
  int res = 1;
  switch (key) {
  case KEY_HELP:
    fprintf(stderr, "Usage: %s mountpoint [options]\n"
        "\n"
        "General options:\n"
        "    -o opt,[opt...]\tmount options\n"
        "    -h, --help\t\tdisplay this help\n"
        "    --version\t\tshow version information\n"
        "    -d, --debug\t\trun in debug mode\n"
        "\n"
        "Mount options:\n"
        "    -b, --basedir DIR\tmount target directory (required)\n"
        "    -H, --host HOST\tmasterd hostname (default: localhost).\n"
        "    -p, --port NUM\tmasterd listen port (default: 9876)\n"
        "    -s, --storage NAME\tchoose a storage manager (choices: posix, "
        "object).\n"
        "\t\t\tdefault: posix.\n"
        "    -w, --dirwidth NUM\tthe directory width for object store.\n"
        "\n"
        , outargs->argv[0]);
    fuse_opt_add_arg(outargs, "-ho");
    fuse_main(outargs->argc, outargs->argv, &opers, NULL);
    exit(1);
  case KEY_VERSION:
    fprintf(stderr, "VSFS version: %s\n", "0.1");
    fuse_opt_add_arg(outargs, "--version");
    fuse_main(outargs->argc, outargs->argv, &opers, NULL);
    exit(1);
  case KEY_DEBUG:
    fuse_opt_add_arg(outargs, "-d");
    res = 0;  // this arg is to be discarded
    break;
  }
  return res;
}

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  fuse_args args = FUSE_ARGS_INIT(argc, argv);
  memset(&options, 0, sizeof(struct options));

  fuse_operations opers;
  opers.access = vsfs_access;
  opers.chmod = vsfs_chmod;
  opers.chown = vsfs_chown;
  opers.create = vsfs_create;
  opers.destroy = vsfs_destroy;
  opers.getattr = vsfs_getattr;
  opers.fgetattr = vsfs_fgetattr;
  opers.ioctl = vsfs_ioctl;
  opers.init = vsfs_init;
  opers.mkdir = vsfs_mkdir;
  opers.open = vsfs_open;
  opers.read = vsfs_read;
  opers.fsync = vsfs_fsync;
  opers.readdir = vsfs_readdir;
  opers.release = vsfs_release;
  opers.readlink = vsfs_readlink;
  opers.rmdir = vsfs_rmdir;
  opers.unlink = vsfs_unlink;
  opers.utimens = vsfs_utimens;
  opers.write = vsfs_write;
  opers.truncate = vsfs_truncate;
  opers.opendir = vsfs_opendir;
  opers.releasedir = vsfs_releasedir;
  opers.flush = vsfs_flush;
  opers.lock = vsfs_lock;
  opers.statfs = vsfs_statfs;
  opers.getxattr = vsfs_getxattr;
#if defined(FUSE_29)
  opers.flock = vsfs_flock;
  // opers.read_buf = vsfs_read_buf;
  opers.read_buf = NULL;
  opers.write_buf = vsfs_write_buf;
#endif

  Status status;
  string host = "localhost";
  string storage = "posix";  // Default storage manager is PosixStorageManager;
  if (fuse_opt_parse(&args, &options, vsfs_opts, vsfs_opt_proc) == -1) {
    status = Status(-1, "Can not parse command line parameters");
    goto exit_handler;
  }

  if (!options.basedir || strlen(options.basedir) == 0) {
    status = Status(-1, "Missing base dir");
    goto exit_handler;
  }
  if (options.master_host && strlen(options.master_host) > 0) {
    host = options.master_host;
  }
  if (options.storage && strlen(options.storage) > 0) {
    storage = options.storage;
  }
  if (storage != "posix" && storage != "object") {
    status = Status(-1, "Wrong storage type.");
    goto exit_handler;
  }
  if (options.master_port == 0) {
    options.master_port = 9876;
  }
  if (options.dirwidth > 0) {
    FLAGS_object_storage_dirwidth = options.dirwidth;
  }
  LOG(INFO) << "VSFS connect to " << options.master_host << ":"
            << options.master_port;
  status = VsfsFuse::init(options.basedir, args.argv[args.argc-1],
                          host, options.master_port, storage);
  if (!status.ok()) {
    goto exit_handler;
  }
  if (fuse_main(args.argc, args.argv, &opers, NULL)) {
    printf("\n");
  };

 exit_handler:
  fuse_opt_free_args(&args);
  if (!status.ok()) {
    fprintf(stderr, "%s\n", status.message().c_str());
  }
  return status.error();
}
