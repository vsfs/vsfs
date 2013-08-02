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

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include "vobla/status.h"
#include "vsfs/client/commands.h"

using std::string;
using std::unique_ptr;
using vobla::Status;
using vsfs::client::Command;
using vsfs::client::HelpCommand;

int main(int argc, char* const argv[]) {
  Command::program = argv[0];

  if (argc == 1) {
    HelpCommand::usage();
    return 1;
  }
  const string subcmd = argv[1];
  unique_ptr<Command> command(Command::create_command(subcmd));
  argc--;
  argv++;
  int ret = command->parse_args(argc, argv);
  if (ret) {
    if (ret == -1) {
      command->print_help();
    }
    return ret;
  }
  Status status = command->run();
  if (!status.ok()) {
    fprintf(stderr, "Failed: %s\n", status.message().c_str());
  }
  return status.error();
}
