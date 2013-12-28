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

#include <boost/python.hpp>
#include <string>
#include "vobla/status.h"
#include "vsfs/api/cpp/vsfs.h"
#include "vsfs/common/storage_manager.h"

BOOST_PYTHON_MODULE(vsfs_py) {
  using namespace boost::python;  // NOLINT

  using vobla::Status;
  class_<Status>("Status", init<int, std::string>())
      .add_property("error", &Status::error, "Status error code")
      .def("set_error", &Status::set_error)
      .def("message", &Status::message,
           return_value_policy<copy_const_reference>())
      .def("set_message", &Status::set_message)
      .def("ok", &Status::ok);

  using vsfs::File;
  // class_<File>("File")

  /*
  using vsfs::StorageManager;
  using vsfs::Vsfs;
  class_<Vsfs>("VsfsPy", init<std::string, int, StorageManager*>())
      .def("connect", &Vsfs::connect);
  */
}
