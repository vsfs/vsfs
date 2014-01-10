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

#ifndef VSFS_UI_CLI_COMMANDS_H_
#define VSFS_UI_CLI_COMMANDS_H_

#include <boost/utility.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace vobla {
class Status;
class Timer;
}

namespace vsfs {
namespace ui {
namespace cli {

using vobla::Status;

/**
 * \class Command
 * \brief Interface of command.
 */
class Command : boost::noncopyable {
 public:
  static const char* program;

  /// Factory method.
  static Command* create_command(const string &subcmd);

  /// Constructor.
  Command();

  /// Destructor.
  virtual ~Command();

  /**
   * \brief Parses sub-command parameters.
   * \return 0 if success. Returns -1 'print_help()' will be called then.
   * Returning other negtive value means the errors have been handled
   * internally.
   */
  virtual int parse_args(int argc, char* const argv[]) = 0;

  /// Runs this command.
  virtual vobla::Status run() = 0;

  /// Print detailed help information for this sub command.
  virtual void print_help() const = 0;

 protected:
  void set_verbose_level(const char* level);

  string host_;
  int port_;
  bool debug_;
  unique_ptr<vobla::Timer> timer_;
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
 * \class IndexCommand
 * \brief Provides an indexing interface to command line.
 */
class IndexCommand : public Command {
 public:
  IndexCommand() = default;

  int parse_args(int argc, char* const argv[]);

  /// Prints the detailed help for "index" command.
  void print_help() const;

  Status run();

 private:
  unique_ptr<Command> sub_command_;
};

/**
 * \brief "vsfs index create..." command.
 */
class IndexCreateCommand : public Command {
 public:
  IndexCreateCommand();

  int parse_args(int argc, char* const argv[]);

  void print_help() const;

  Status run();

 private:
  /// Index root path.
  string root_;

  /// Index name.
  string name_;

  /// The data structure type of index. (e.g., btree or hash).
  int index_type_;

  /// The type of the key of index. (e.g., int32, float or string).
  int key_type_;
};

/**
 * \brief "vsfs index destory" command.
 *
 * Completely deletes a file index.
 */
class IndexDestroyCommand : public Command {
 public:
  IndexDestroyCommand() = default;

  int parse_args(int argc, char* const argv[]);

  void print_help() const;

  Status run();

 private:
  /// Index root path.
  string root_;

  /// Index name.
  string name_;
};

/**
 * \brief "vsfs index insert/remove" command.
 *
 * It manages both "insert" and "remove" sub-commands.
 */
class IndexUpdateCommand : public Command {
 public:
  enum IndexOp {
    UPDATE, REMOVE
  };

  explicit IndexUpdateCommand(IndexOp op);

  int parse_args(int argc, char* const argv[]);

  void print_help() const;

  Status run();

  IndexOp op() const;

  /**
   * \brief Parses an index record from stdin.
   * \param[in] line a line contains "/file/path key"
   * \param[out] path filled with file path.
   * \param[out] key filled with file key in the index.
   * \return true if the line is correctly parsed.
   */
  bool parse_record(const string& line, string* path, string* key) const;

 private:
  typedef unordered_map<string, vector<string>> IndexDataMap;

  IndexOp op_;

  /// Index name.
  string name_;

  /// Set to true to use stdin to feed.
  bool use_stdin_;

  uint64_t batch_size_;

  IndexDataMap index_data_;
};

}  // namespace cli
}  // namespace ui
}  // namespace vsfs

#endif  // VSFS_UI_CLI_COMMANDS_H_
