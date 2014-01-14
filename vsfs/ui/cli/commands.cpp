/*
 * Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
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

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include "vobla/status.h"
#include "vobla/string_util.h"
#include "vobla/traits.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/common/posix_path.h"
#include "vsfs/index/index_info.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/thrift_utils.h"
#include "vsfs/ui/cli/commands.h"

using boost::bad_lexical_cast;
using boost::lexical_cast;
using std::string;
using std::unique_ptr;
using std::vector;
using vobla::Status;
using vobla::stringprintf;
using vobla::tokenize;
using vsfs::client::VSFSRpcClient;
using vsfs::index::IndexInfo;

namespace fs = boost::filesystem;

#define ERROR_MSG_RETURN(Ret, Msg) do { \
    fprintf(stderr, Msg); \
    return Ret; } \
    while (false);  // NOLINT

namespace vsfs {
namespace ui {
namespace cli {

const char *Command::program = NULL;
const char *kDefaultMasterHost = "localhost";
const int kDefaultMasterPort = 9876;
const uint64_t kDefaultBatchSize = 10000;

/**
 * \class SearchCommand
 * \brief Search files according to the complex query.
 */
class SearchCommand : public Command {
 public:
  int parse_args(int argc, char* argv[]);

  /// Print detailed help for "search" command.
  void print_help() const;

  /// Run search function.
  Status run();

 private:
  string query_;
  double master_time_;
  double nodes_time_;
};

Command* Command::create_command(const string &subcmd) {
  if (subcmd == "help" || subcmd == "-h" || subcmd == "--help") {
    return new HelpCommand;
  } else if (subcmd == "search") {
    return new SearchCommand;
  } else if (subcmd == "index") {
    return new IndexCommand;
  }
  fprintf(stderr, "Error: Unknown command: %s\n", subcmd.c_str());
  HelpCommand::usage();
  return nullptr;
}

Command::Command() : host_(kDefaultMasterHost), port_(kDefaultMasterPort),
    debug_(false) {
  char* env = getenv("VSFS_HOST");
  if (env) {
    host_ = env;
  }
  env = getenv("VSFS_PORT");
  if (env) {
    try {
      port_ = lexical_cast<int>(env);
    } catch (bad_lexical_cast e) {
      LOG(ERROR) << "Bad port number: " << env;
    }
  }
}

Command::~Command() {
}

void Command::set_verbose_level(const char* level) {
  FLAGS_logtostderr = true;
  int verbose_level = 0;
  if (level) {
    string level_str(level);
    verbose_level = lexical_cast<int>(level_str);
  }
  VLOG(0) << "Sets verbose level to " << verbose_level;
  FLAGS_v = verbose_level;
}

int HelpCommand::parse_args(int argc, char* argv[]) {
  if (argc > 1) {
    sub_command_ = argv[1];
    return 0;
  }
  return -1;
}

void HelpCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs {-h|--help} command [options] [ARGS]\n");
  fprintf(stderr, "Global Options:\n"
          "  -h, --help\t\tdisplay this help information.\n"
          "Supported Commands\n"
          "  help\t\t\tprint detail help on each command.\n"
          "  search\t\trun complex query.\n"
          "  index\t\t\tindex files.\n"
          "\nFurther help:\n"
          "  man vsfs\n"
          "");
}

Status HelpCommand::run() {
  if (sub_command_.empty()) {
    print_help();
  } else if (sub_command_ == "search") {
    SearchCommand().print_help();
  } else if (sub_command_ == "index") {
    IndexCommand().print_help();
  } else {
    fprintf(stderr, "Error: unknown command: %s.\n", sub_command_.c_str());
    return Status(-1, "Unknown command");
  }
  return Status::OK;
}

void HelpCommand::usage() {
  HelpCommand help;
  help.run();
}

// --- SearchCommand -----
int SearchCommand::parse_args(int argc, char* argv[]) {
  int ch;
  static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "debug", no_argument, NULL, 1 },
    { "port", required_argument, NULL, 'p' },
    { "host", required_argument, NULL, 'H' },
    { "verbose", optional_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 }
  };
  static const char* shortopts = "hp:H:v:";
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 1:
        debug_ = 1;
        break;
      case 'p':
        port_ = lexical_cast<int>(optarg);
        break;
      case 'H':
        host_ = optarg;
        break;
      case 'v':
        set_verbose_level(optarg);
        break;
      case 'h':
      default:
        return -1;
    }
  }
  argc -= optind;
  argv += optind;

  if (!argc) {
    return -1;
  }
  query_ = argv[0];
  return 0;
}

void SearchCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs search [options] QUERY\n");
  fprintf(stderr, "Options:\n"
          "  -h, --help\t\t\tdisplay this help information.\n"
          "  -d, --debug\t\t\trun in debug mode.\n"
          "  -v, --verbose[=LEVEL]\t\tRun in verbose mode.\n"
          "  -H, --host STR\t\tset the address of the master node.\n"
          "  -p, --port NUM\t\tset the port of the master node.\n"
          "  -r, --root PATH\t\tsets the root path of file query.\n"
          "  --profile\t\t\tset to profile operation.\n"
          "\n");
}

Status SearchCommand::run() {
  VLOG(1) << "SearchCommand::run: Issue query to master: " << query_.c_str();
  ComplexQuery query;
  auto status = query.parse(query_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to parse command line query: " << query_;
    return status;
  }

  VSFSRpcClient client(host_, port_);
  status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
               << status.message();
    return status;
  }

  vector<string> files;
  status = client.search(query, &files);
  for (const auto& file_path : files) {
    printf("%s\n", file_path.c_str());
  }
  return Status::OK;
}

int IndexCommand::parse_args(int argc, char* argv[]) {
  sub_command_.reset();
  string subcmd = "";
  if (argc == 1) {
    return -1;
  }
  if (argc > 1) {
    subcmd = argv[1];
  }
  if (subcmd == "-h" || subcmd == "--help") {
    return -1;
  }

  if (subcmd == "create") {
    sub_command_.reset(new IndexCreateCommand);
  } else if (subcmd == "destroy") {
    sub_command_.reset(new IndexDestroyCommand);
  } else if (subcmd == "insert") {
    sub_command_.reset(new IndexUpdateCommand(IndexUpdateCommand::UPDATE));
  } else if (subcmd == "remove") {
    sub_command_.reset(new IndexUpdateCommand(IndexUpdateCommand::REMOVE));
  } else if (subcmd == "stat") {
  } else if (subcmd == "list") {
    sub_command_.reset(new IndexListCommand);
  }
  if (!sub_command_) {
    fprintf(stderr, "Unknown command: %s.\n", subcmd.c_str());
    return -1;
  }
  return sub_command_->parse_args(argc - 1, argv + 1);
}

void IndexCommand::print_help() const {
  if (sub_command_) {
    sub_command_->print_help();
    return;
  }
  fprintf(stderr, "Usage: vsfs index {create|destroy|insert|remove|stat|list} "
                  "[options] ARG...\n"
                  "\nFurther help:\n"
                  "  man vsfs\n"
                  "");
}

Status IndexCommand::run() {
  if (sub_command_) {
    return sub_command_->run();
  }
  return Status(-EINVAL, "Invalid command");
}

IndexCreateCommand::IndexCreateCommand() : index_type_(0), key_type_(0) {
}

int IndexCreateCommand::parse_args(int argc, char* argv[]) {
  optind = 1;
  static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "type", required_argument, NULL, 't' },
    { "key", required_argument, NULL, 'k' },
    { "debug", no_argument, NULL, 'd' },
    { "verbose", optional_argument, NULL, 'v' },
    { "host", required_argument, NULL, 'H' },
    { "port", required_argument, NULL, 'p' },
    { NULL, 0, NULL, 0 }
  };
  static const char* shortopts = "ht:k:dv:H:p:";
  int ch;
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 't':
        index_type_ = IndexInfo::string_to_index_type(optarg);
        break;
      case 'k':
        key_type_ = parse_type_string_to_int(optarg);
        break;
      case 'd':
        debug_ = true;
        break;
      case 'v':
        set_verbose_level(optarg);
        break;
      case 'H':
        host_ = optarg;
        break;
      case 'p':
        port_ = lexical_cast<int>(optarg);
        break;
      case 'h':
      default:
        return -1;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 0) {
    return -1;
  }
  if (index_type_ == -1) {
    ERROR_MSG_RETURN(-1, "Wrong index type.\n");
  }
  if (key_type_ == -1) {
    ERROR_MSG_RETURN(-1, "Wrong key type.\n");
  }
  if (argc < 2) {
    ERROR_MSG_RETURN(-1, "Miss index directory or name.\n");
  }
  root_ = argv[0];
  name_ = argv[1];
  return 0;
}

void IndexCreateCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs index create [options] DIR NAME\n");
  fprintf(stderr, "Options:\n"
          "  -h, --help\t\t\tShow this help.\n"
          "  --debug\t\t\tRun in debug mode.\n"
          "  -v, --verbose[=LEVEL]\t\tRun in verbose mode and level.\n"
          "  -H, --host\t\t\tSet the master address.\n"
          "  -p, --port\t\t\tSet the master port.\n"
          "  -t, --type TYPE\t\tSet the index type (btree, hash).\n"
          "  -k, --key TYPE\t\tSet the key type ({u}int{8,16,32,64}, "
          "float, double, string).\n"
          "\n\nFurther help:\n"
          "  man vsfs\n"
          "");
}

Status IndexCreateCommand::run() {
  // string canonical_root = fs::absolute(index_root_).string();
  string canonical_root = root_;
  VLOG(0) << "Creating index...";
  VLOG(0) << "Index: " << canonical_root << ":" << name_
      << " type: " << index_type_ << " "
      << " key: " << key_type_;
  VSFSRpcClient client(host_, port_);
  Status status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
        << status.message();
    return status;
  }
  return client.create_index(canonical_root, name_,
                             index_type_, key_type_,
                             0755, getuid(), getgid());
}


void IndexDestroyCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs index destroy [options] DIR NAME\n");
  fprintf(stderr, "Options:\n"
          "  -h, --help\t\t\tShow this help.\n"
          "  --debug\t\t\tRun in debug mode.\n"
          "  -v, --verbose[=LEVEL]\t\tRun in verbose mode and level.\n"
          "  -H, --host\t\t\tSet the master address.\n"
          "  -p, --port\t\t\tSet the master port.\n"
          "\n\nFurther help:\n"
          "  man vsfs\n"
          "");
}

int IndexDestroyCommand::parse_args(int argc, char* argv[]) {
  optind = 1;
  static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "debug", no_argument, NULL, 'd' },
    { "verbose", optional_argument, NULL, 'v' },
    { "host", required_argument, NULL, 'H' },
    { "port", required_argument, NULL, 'p' },
    { NULL, 0, NULL, 0 }
  };
  static const char* shortopts = "ht:k:dv:H:p:";
  int ch;
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 'd':
        debug_ = true;
        break;
      case 'v':
        set_verbose_level(optarg);
        break;
      case 'H':
        host_ = optarg;
        break;
      case 'p':
        port_ = lexical_cast<int>(optarg);
        break;
      case 'h':
      default:
        return -1;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 0) {
    return -1;
  }
  if (argc < 2) {
    ERROR_MSG_RETURN(-1, "Miss index directory or name.\n");
  }
  root_ = argv[0];
  name_ = argv[1];
  return 0;
}

Status IndexDestroyCommand::run() {
  VLOG(0) << "Destroying index: " << root_ << "," << name_ << "..";
  VSFSRpcClient client(host_, port_);
  auto status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
        << status.message();
    return status;
  }
  return client.remove_index(root_, name_);
}

IndexUpdateCommand::IndexUpdateCommand(IndexOp op) : op_(op) {
}

void IndexUpdateCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs index %s [options] NAME [FILE KEY]...\n",
          op_ == IndexOp::UPDATE ? "insert" : "remove");
  fprintf(stderr, "General Options:\n"
          "  -h, --help\t\t\tShow this help.\n"
          "  --debug\t\t\tRun in debug mode.\n"
          "  -v, --verbose[=LEVEL]\t\tRun in verbose mode and level.\n"
          "  -H, --host\t\t\tSet the master address.\n"
          "  -p, --port\t\t\tSet the master port.\n"
          "  -s, --stdin\t\t\tRead indexing metadata from stdin.\n"
          "  -b, --batch NUM\t\tSet the batch size to send records.\n"
          // "  --profile [masterd,indexd]\tSet to profile operation.\n"
          "\n\nFurther help:\n"
          "  man vsfs\n"
          "");
}

int IndexUpdateCommand::parse_args(int argc, char* argv[]) {
  // Reset optind to 1 to allow getopt_long() being called multiple times.
  optind = 1;
  static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "stdin", no_argument, NULL, 's' },
    { "debug", no_argument, NULL, 'd' },
    { "verbose", optional_argument, NULL, 'v' },
    { "host", required_argument, NULL, 'H' },
    { "port", required_argument, NULL, 'p' },
    // { "profile", optional_argument, NULL, 4 },
    { "batch", required_argument, NULL, 'b' },
    { NULL, 0, NULL, 0 },
  };
  static const char* shortopts = "hsdv:H:p:b:";

  int ch;
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 's':
        use_stdin_ = true;
        break;
      case 'd':
        debug_ = true;
        break;
      case 'v':
        set_verbose_level(optarg);
        break;
      case 'H':
        host_ = optarg;
        break;
      case 'p':
        port_ = lexical_cast<int>(optarg);
        break;
      case 'b':
        batch_size_ = lexical_cast<uint64_t>(optarg);
        break;
      case 'h':
      default:
        return -1;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 0) {
    return -1;
  }
  name_ = argv[0];
  if (argc > 1 && argc % 2 == 0) {
    LOG(ERROR) << "Missing FILE, KEY pairs.";
    return -1;
  }
  for (int i = 1; i < argc; i += 2) {
    index_data_[argv[i]].push_back(argv[i + 1]);
  }
  return 0;
}

bool IndexUpdateCommand::parse_record(
    const string& buf, string* path, string* key) const {
  CHECK_NOTNULL(path);
  CHECK_NOTNULL(key);
  if (buf.empty()) {
    return false;
  }
  vector<string> params = tokenize(buf);
  if (params.size() != 2) {
    return false;
  }
  *path = params[0];
  *key = params[1];
  return true;
}

Status IndexUpdateCommand::run() {
  VSFSRpcClient client(host_, port_);
  Status status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
        << status.message();
    return status;
  }

  int op = op_ == UPDATE ? VSFSRpcClient::IndexUpdateRequest::INSERT
                         : VSFSRpcClient::IndexUpdateRequest::REMOVE;
  vector<VSFSRpcClient::IndexUpdateRequest> updates;
  if (use_stdin_) {
    // Reads record through stdin
    string buf;
    while (!std::cin.eof()) {
      std::getline(std::cin, buf);
      if (buf.empty()) {
        continue;
      }
      string file_path, key;
      if (!parse_record(buf, &file_path, &key)) {
        LOG(ERROR) << "An error has occorred when read: " << buf;
        return Status(-EINVAL, "Wrong index record format.");
      }
      updates.emplace_back();
      auto& request = updates.back();
      request.op = op;
      request.file_path = fs::absolute(file_path).string();
      request.index_name = name_;
      request.key = key;

      if (updates.size() >= static_cast<size_t>(batch_size_)) {
        status = client.update(updates);
        if (!status.ok()) {
          LOG(ERROR) << "Failed to update index: " << status.message();
          return status;
        }
        updates.clear();
      }
    }
  } else {
    // Pass records through command line parameters.
    for (const auto& update : index_data_) {
      for (const auto& key : update.second) {
        updates.emplace_back();
        auto& request = updates.back();
        request.op = op;
        request.file_path = fs::absolute(update.first).string();
        request.index_name = name_;
        request.key = key;
      }
    }
    status = client.update(updates);
    updates.clear();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to update index: " << status.message();
    }
  }
  if (!updates.empty()) {
    status = client.update(updates);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to update index: " << status.message();
    }
  }
  return status;
}

// --- Index list command
IndexListCommand::IndexListCommand() : recursive_(false) {
}

void IndexListCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs index list [options] PATH\n");
  fprintf(stderr, "Options:\n"
          "  -h, --help\t\t\tShow this help.\n"
          "  -H, --host\t\t\tSet the master address.\n"
          "  -p, --port\t\t\tSet the master port.\n"
          "  -r, --recursive\t\tRecusively list out all indices."
          "\n\nFurther help:\n"
          "  man vsfs\n"
          "");
}

int IndexListCommand::parse_args(int argc, char* argv[]) {
  static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "port", required_argument, NULL, 'p' },
    { "host", required_argument, NULL, 'H' },
    { "recursive", no_argument, NULL, 'r' },
    { NULL, 0, NULL, 0 }
  };
  static const char* shortopts = "hp:H:v:r";
  int ch;
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 'p':
        port_ = lexical_cast<int>(optarg);
        break;
      case 'H':
        host_ = optarg;
        break;
      case 'r':
        recursive_ = true;
        break;
      case 'h':
      default:
        return -1;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 0) {
    return -1;
  }
  path_ = argv[0];
  return 0;
}

namespace {

Status print_index_info(VSFSRpcClient* client, const string& path,
                        bool recursive) {
  CHECK_NOTNULL(client);
  Status status;
  if (recursive) {  // depth-first.
    vector<string> subfiles;
    status = client->readdir(path, &subfiles);
    if (!status.ok()) {
      VLOG(1) << "Failed to readdir() on " << path << ": " << status.message();
      return status;
    }
    struct stat stbuf;
    for (const auto& subfile : subfiles) {
      string subfile_path = path + "/" + subfile;
      status = client->getattr(subfile_path, &stbuf);
      if (!status.ok()) {
        VLOG(1) << "Failed to getattr of " << subfile_path
                << ": " << status.message();
        return status;
      }
      if (S_ISDIR(stbuf.st_mode) && !S_ISLNK(stbuf.st_mode)) {
        // Recusively go to next level of directory but not follow symlinks.
        status = print_index_info(client, subfile_path, recursive);
        if (!status.ok()) {
          return status;
        }
      }
    }
  }

  vector<IndexInfo> index_infos;
  printf("Indices on: %s\n", path.c_str());
  status = client->info(path, &index_infos);
  if (!status.ok()) {
    fprintf(stderr, "Error to query the index info for %s: %s.\n",
            path.c_str(), status.message().c_str());
    return status;
  }
  for (const auto& info : index_infos) {
    // TODO(eddyxu): better format for list output.
    printf("  - %s (%s, %s)\n",
           info.index_name().c_str(),
           info.index_type_string().c_str(),
           info.key_type_string().c_str());
  }
  return Status::OK;
}

}   // namespace

Status IndexListCommand::run() {
  VSFSRpcClient client(host_, port_);
  auto status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
               << status.message();
    return status;
  }

  return print_index_info(&client, path_, recursive_);
}

}  // namespace cli
}  // namespace ui
}  // namespace vsfs
