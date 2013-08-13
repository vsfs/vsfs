/*
 * Copyright 2012 (c) Lei Xu <eddyxu@gmail.com>
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

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <signal.h>
#include <unistd.h>
#include <memory>
#include <string>
#include "vobla/status.h"
#include "vsfs/masterd/master_controller.h"

using vsfs::masterd::MasterController;
using std::string;

DEFINE_bool(daemon, false, "Runs in daemon mode.");
DEFINE_bool(primary, false, "Sets this node as primary master node.");
DEFINE_string(pidfile, "", "Sets the pid file.");
DEFINE_string(dir, ".", "Sets the directory to store metadata.");
DEFINE_int32(port, 9876, "Sets the listening port.");

unique_ptr<MasterController> controller;

void signal_handle(int sign) {
  (void) sign;
  if (controller) {
    controller->stop();
  }
}

int main(int argc, char *argv[]) {
  string usage = "Usage: masterd [options]";
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  signal(SIGTERM, signal_handle);
  signal(SIGINT, signal_handle);

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

  controller.reset(new MasterController(FLAGS_dir, "", FLAGS_port,
                                        FLAGS_primary));
  auto status = controller->init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to open metadata db: " << status.message();
    return status.error();
  }
  controller->start();
  return 0;
}
