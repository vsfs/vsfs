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

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <string>
#include "vsfs/indexd/index_controller.h"

using std::string;
using std::unique_ptr;
using vsfs::indexd::IndexController;

DEFINE_string(pidfile, "",
              "Sets the file to store the pid of this controller.");
DEFINE_string(datadir, ".", "Sets the data directory for storing indices.");
DEFINE_bool(daemon, false, "Run in daemon mode.");

unique_ptr<IndexController> controller;

void signal_handle(int sign) {
  (void) sign;
  if (controller) {
    LOG(INFO) << "Shutting down IndexController...";
    controller->stop();
  }
}

int main(int argc, char *argv[]) {
  const string usage = "Usage: indexd [options]";
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (FLAGS_daemon) {
    LOG(INFO) << "Deamonlize.";
    if (daemon(1, 0) == -1) {
      LOG(ERROR) << "Failed to daemonlize it.";
    }
  }

  if (!FLAGS_pidfile.empty()) {
    FILE* pidfp = fopen(FLAGS_pidfile.c_str(), "w");
    fprintf(pidfp, "%d\n", getpid());
    fclose(pidfp);
  }

  signal(SIGTERM, signal_handle);
  signal(SIGINT, signal_handle);

  controller.reset(new IndexController(FLAGS_datadir));
  controller->start();
  LOG(INFO) << "Done";
  return 0;
}
