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

#ifndef VSFS_CLIENT_COMMANDS_H_
#define VSFS_CLIENT_COMMANDS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "vsfs/rpc/IndexServer.h"
#include "vsfs/rpc/MasterServer.h"
#include "vsfs/rpc/rpc_client.h"

using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace vobla {
class Timer;
class Status;
}

namespace vsfs {
namespace client {

/**
 * \class Command
 * \brief Interface of command.
 */
class Command {
  typedef rpc::RpcClient<MasterServerClient> MasterClientType;
  typedef rpc::RpcClient<IndexServerClient> NodeClientType;

 public:
  static const char* program;

  /// Factory method.
  static Command* create_command(const string &subcmd);

  Command();

  virtual ~Command();

  /**
   * \brief Parses sub-command parameters.
   * \return 0 if success. Returns -1 'print_help()' will be called then.
   * Returning other negtive value means the errors have been handled
   * internally.
   */
  virtual int parse_args(int argc, char* const argv[]) = 0;

  virtual vobla::Status run() = 0;

  virtual void print_help() const = 0;

 protected:
  void set_verbose_level(const char* level);

  unique_ptr<MasterClientType> master_client_;

  string host_;
  int port_;
  bool debug_;
  unique_ptr<vobla::Timer> timer_;

  void init_master_client();
};

/**
 * \class HelpCommand
 * \brief print out help message.
 */
class HelpCommand : public Command {
 public:
  int parse_args(int argc, char* const argv[]);

  void print_help() const;

  vobla::Status run();

  static void usage();

 private:
  string sub_command_;
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
  vobla::Status run();

 private:
  string query_;
  double master_time_;
  double nodes_time_;
};

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

  vobla::Status run();

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

  vobla::Status create_index();

  vobla::Status update_index();

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

  IndexDataMap index_data_;
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

  vobla::Status run();

 private:
  bool recursive_;

  vector<string> dirs_;
};

}  // namespace client
}  // namespace vsfs

#endif  // VSFS_CLIENT_COMMANDS_H_
