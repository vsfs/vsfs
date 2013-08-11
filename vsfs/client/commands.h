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

#include <boost/utility.hpp>
#include <string>
#include <memory>

using std::string;
using std::unique_ptr;

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

}  // namespace client
}  // namespace vsfs

#endif  // VSFS_CLIENT_COMMANDS_H_
