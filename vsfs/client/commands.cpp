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

/**
 * \file commands.cpp
 * \brief implemtation of distributed commands.
 */

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <functional>
#include <iostream>  // NOLINT
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "vobla/stl_util.h"
#include "vobla/status.h"
#include "vobla/timer.h"
#include "vobla/traits.h"
#include "vsfs/client/commands.h"
#include "vsfs/client/vsfs_rpc_client.h"
#include "vsfs/common/complex_query.h"
#include "vsfs/common/posix_path.h"
#include "vsfs/index/index_info.h"
#include "vsfs/rpc/rpc_client.h"
#include "vsfs/rpc/thrift_utils.h"

using boost::lexical_cast;
using std::list;
using std::string;
using std::thread;
using std::unique_ptr;
using std::vector;
using vobla::Status;
using vobla::Timer;
using vsfs::index::IndexInfo;
namespace fs = boost::filesystem;

#define ERROR_MSG_RETURN(Ret, Msg) do { \
    fprintf(stderr, Msg); \
    return Ret; } \
    while (false);  // NOLINT

namespace vsfs {
namespace client {

const char *Command::program = NULL;
const char *kDefaultMasterHost = "localhost";
const int kDefaultMasterPort = 9876;
const uint64_t kDefaultBatchSize = 10000;

/**
 * \class IndexCommand
 * \brief Provides an indexing interface to command line.
 */
class IndexCommand : public Command {
 public:
  IndexCommand();

  int parse_args(int argc, char* const argv[]);

  /// Prints the detailed help for "index" command.
  void print_help() const;

  Status run();

 private:
  /**
   * \brief A hash mapping from the file name to a vector of indexing keys.
   *
   * E.g. <file name, [key0, key1, key2, ...]>
   */
  typedef unordered_map<string, vector<string>> IndexDataMap;

  enum IndexOp {
    /// Adds to an index entry
    ADD = 1,
    /// Updates an index entry.
    UPDATE,
    /// Deletes from an index entry.
    DELETE
  };

  enum Operation {
    /// Updates on an existing index.
    INDEX = 1,
    /// Creates a named index.
    CREATE_INDEX,
    /// Deletes a named index.
    DELETE_INDEX,
    /// Queries the information of the named index.
    INFO
  };

  /// Prints out examples of usage.
  void show_examples() const;

  Status create_index();

  Status update_index();

  string index_root_;

  /// The name of named index.
  string index_name_;

  /// Set to true to use stdin to feed.
  bool use_stdin_;

  /// Operations.
  int operation_;

  /// Operations on index.
  int index_op_;

  /// The data structure type of index. (e.g., btree or hash).
  int index_type_;

  /// The type of the key of index. (e.g., int32, float or string).
  int key_type_;

  uint64_t batch_size_;

  IndexDataMap index_data_;
};

/**
 * \class SearchCommand
 * \brief Search files according to the complex query.
 */
class SearchCommand : public Command {
 public:
  int parse_args(int argc, char* const argv[]);

  /// Print detailed help for "search" command.
  void print_help() const;

  /// Run search function.
  Status run();

 private:
  string query_;
  double master_time_;
  double nodes_time_;
};

/**
 * \class InfoCommand
 * \brief Query the information of various aspects of VSFS.
 */
class InfoCommand : public Command {
 public:
  InfoCommand();

  int parse_args(int argc, char* const argv[]);

  void print_help() const;

  Status run();

 private:
  bool recursive_;

  vector<string> dirs_;
};

Command* Command::create_command(const string &subcmd) {
  if (subcmd == "help" || subcmd == "-h" || subcmd == "--help") {
    return new HelpCommand;
  } else if (subcmd == "search") {
    return new SearchCommand;
  } else if (subcmd == "index") {
    return new IndexCommand;
  } else if (subcmd == "info") {
    return new InfoCommand;
  }
  fprintf(stderr, "Error: Unknown command: %s\n", subcmd.c_str());
  HelpCommand::usage();
  return nullptr;
}

Command::Command() : host_(kDefaultMasterHost), port_(kDefaultMasterPort),
    debug_(false) {
  timer_.reset(new Timer);
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

int HelpCommand::parse_args(int argc, char* const argv[]) {
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
          "  info\t\t\tquery the index information.\n"
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
int SearchCommand::parse_args(int argc, char* const argv[]) {
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

// ----------- IndexCommand ------------
IndexCommand::IndexCommand()
  : use_stdin_(false), operation_(INDEX), index_op_(ADD),
    batch_size_(kDefaultBatchSize) {
}

int IndexCommand::parse_args(int argc, char* const argv[]) {
  static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "examples", no_argument, NULL, 1 },
    { "create", no_argument, NULL, 'c' },
    { "delete", no_argument, NULL, 'd' },
    { "info", no_argument, NULL, 2 },
    { "stdin", no_argument, NULL, 's' },
    { "name", required_argument, NULL, 'n' },
    { "type", required_argument, NULL, 't' },
    { "key", required_argument, NULL, 'k' },
    { "debug", no_argument, NULL, 3 },
    { "verbose", optional_argument, NULL, 'v' },
    { "host", required_argument, NULL, 'H' },
    { "port", required_argument, NULL, 'p' },
    { "profile", optional_argument, NULL, 4 },
    { "batch", required_argument, NULL, 'b' },
    { NULL, 0, NULL, 0 }
  };
  static const char* shortopts = "ha:cdsn:t:k:v:H:p:b:";

  bool do_create = false;
  bool do_info = false;
  bool do_delete = false;
  string profile_targets;
  int ch;
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 1:  // Shows examples
        show_examples();
        return -2;
      case 's':
        use_stdin_ = true;
        break;
      case 'n':
        index_name_ = optarg;
        break;
      case 'c':
        do_create = true;
        break;
      case 'd':
        do_delete = true;
        break;
      case 2:
        do_info = true;
        break;
      case 't':
        index_type_ = IndexInfo::string_to_index_type(optarg);
        break;
      case 'k':
        key_type_ = parse_type_string_to_int(optarg);
        LOG(INFO) << "Parse key type: " << optarg << " to: " << key_type_;
        break;
      case 3:
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

  // Sanity checks.
  if (index_name_.empty()) {
    ERROR_MSG_RETURN(-1, "You must provide index name.\n");
  }
  if ((do_create && do_delete) || (do_create && do_info) ||
      (do_delete && do_info)) {
    ERROR_MSG_RETURN(-1, "You should provide none, or only one of --create, "
                     "--delete or --info.\n");
  }
  if (do_create) {
    operation_ = CREATE_INDEX;
    if (index_type_ == -1) {
      fprintf(stderr, "Wrong index type.\n");
      return -1;
    } else if (key_type_ == -1) {
      ERROR_MSG_RETURN(-1, "Wrong key type.\n");
    }
    if (argc < 1) {
      ERROR_MSG_RETURN(-1, "Miss index path.\n");
    }
    index_root_ = argv[0];
    return 0;
  } else if (do_delete) {
    operation_ = DELETE_INDEX;
  } else if (do_info) {
    operation_ = INFO;
  }

  if (argc == 0) {
    use_stdin_ = true;
  } else if (argc > 0) {
    if (argc % 2 != 0) {
      ERROR_MSG_RETURN(-1, "The files and keys are not match.\n");
    }

    int count = 0;
    while (count < argc) {
      index_data_[argv[count]].push_back(argv[count+1]);
      count += 2;
    }
  }

  return 0;
}

void IndexCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs index [options] [[FILE KEY], ...]\n");
  fprintf(stderr, "Options:\n"
          "  -h, --help\t\t\tShow this help.\n"
          "  --debug\t\t\tRun in debug mode.\n"
          "  -v, --verbose[=LEVEL]\t\tRun in verbose mode and level.\n"
          "  --examples\t\t\tShow some examples of the usage.\n"
          "  -H, --host\t\t\tSet the master address.\n"
          "  -p, --port\t\t\tSet the master port.\n"
          "  -c, --create\t\t\tCreate a named index.\n"
          "  --info\t\t\tShow the information of the named index.\n"
          "  -s, --stdin\t\t\tRead indexing metadata from stdin.\n"
          "  -n, --name NAME\t\tSet the name of index to feed.\n"
          "  -t, --type TYPE\t\tSet the index type (btree, hash).\n"
          "\t\t\t\tOnly be used when --create is set.\n"
          "  -k, --key TYPE\t\tSet the key type ({u}int{8,16,32,64}, "
          "float, double, string).\n"
          "\t\t\t\tOnly be used when --create is set.\n"
          "  -b, --batch NUM\t\tSets the batch size to send records.\n"
          "  --profile [masterd,indexd]\tSet to profile operation.\n"
          "");
}

Status IndexCommand::run() {
  if (operation_ == CREATE_INDEX) {
    return create_index();
  } else if (operation_ == INDEX) {
    return update_index();
  }
  return Status::OK;
}

void IndexCommand::show_examples() const {
  fprintf(stderr, "Usage: vsfs index [options] [FILE...]\n");
  fprintf(stderr, "\nSome examples of VSFS index usage:\n"
          " * Creates an index:\n"
          "   $ vsfs index --create --name energy --index-type btree "
          "--key-type float /home/john\n"
          "\n"
          " * Updates 'symbol' index for a single file:\n\n"
          "   $ vsfs index -n symbol /foo/bar/main.cpp awesome_func\n"
          "");
}

Status IndexCommand::create_index() {
  // string canonical_root = fs::absolute(index_root_).string();
  string canonical_root = index_root_;
  LOG(INFO) << "Creating index...";
  LOG(INFO) << "Index: " << canonical_root << ":" << index_name_
            << " type: " << index_type_ << " "
            << " key: " << key_type_;
  VSFSRpcClient client(host_, port_);
  Status status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
               << status.message();
    return status;
  }
  return client.create_index(canonical_root, index_name_,
                             index_type_, key_type_,
                             0755, getuid(), getgid());
}

Status IndexCommand::update_index() {
  VSFSRpcClient client(host_, port_);
  Status status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
        << status.message();
    return status;
  }

  // TODO(lxu): only support insert record to prove of concept.
  vector<VSFSRpcClient::IndexUpdateRequest> updates;
  if (use_stdin_) {
    // Reads record through stdin
    string buf;
    while (!std::cin.eof()) {
      std::getline(std::cin, buf);
      if (buf.empty()) {
        continue;
      }
      // TODO(lxu): handles more complated strings, such as string with
      // quotations.
      size_t sep_pos = buf.find_first_of(" \t");
      size_t value_pos = buf.find_first_not_of(" \t", sep_pos);
      if (sep_pos == string::npos || value_pos == string::npos) {
        LOG(ERROR) << "An error has occorred when read: "
            << buf;
        return Status(-EINVAL, "Wrong index record format.");
      }
      string file_path = buf.substr(0, sep_pos);
      string key = buf.substr(value_pos);
      updates.emplace_back();
      auto& request = updates.back();
      request.op = VSFSRpcClient::IndexUpdateRequest::INSERT;
      request.file_path = fs::absolute(file_path).string();
      request.index_name = index_name_;
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
        request.op = VSFSRpcClient::IndexUpdateRequest::INSERT;
        request.file_path = fs::absolute(update.first).string();
        request.index_name = index_name_;
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

// ------- InfoCommand
InfoCommand::InfoCommand() : recursive_(false) {
}

void InfoCommand::print_help() const {
  fprintf(stderr, "Usage: vsfs info [options] PATH...\n"
          "Options:\n"
          "  -h, --help\t\t\tdisplay this help information.\n"
          "  -r, --recursive\t\tshow all indices recursively.\n"
          "  -d, --detail\t\t\tshow detail status of the index.\n"
          "  -H, --host\t\t\tsets the master address.\n"
          "  -p, --port\t\t\tsets the master port number.\n"
          "\n");
}

int InfoCommand::parse_args(int argc, char* const argv[]) {
  int ch;
  static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "port", required_argument, NULL, 'p' },
    { "host", required_argument, NULL, 'H' },
    { "recursive", no_argument, NULL, 'r' },
    { NULL, 0, NULL, 0 }
  };
  static const char* shortopts = "hp:H:r:";
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 'H':
        host_ = optarg;
        break;
      case 'p':
        port_ = lexical_cast<int>(optarg);
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
  for (int i = 0; i < argc; ++i) {
    dirs_.push_back(argv[i]);
  }
  return 0;
}

Status InfoCommand::run() {
  Status status;
  vector<IndexInfo> index_infos;

  VSFSRpcClient client(host_, port_);
  status = client.init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init connection to master node: "
               << status.message();
    return status;
  }

  // Only query the first directory for now.
  for (const auto& path : dirs_) {
    index_infos.clear();
    printf("Indices on: %s\n", path.c_str());
    status = client.info(path, &index_infos);
    if (!status.ok()) {
      printf("Error to query the index info for %s: %s.\n",
             path.c_str(), status.message().c_str());
    }
    for (const auto& info : index_infos) {
      printf("  - %s\n", info.index_name().c_str());
    }
  }
  return Status::OK;
}

}  // namespace client
}  // namespace vsfs
