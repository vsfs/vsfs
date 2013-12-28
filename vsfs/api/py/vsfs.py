#!/usr/bin/env python
#
# Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import vsfs_py


class File(object):
    """Behavior like python's file.
    """
    def __init__(self):
        pass

    def __enter__(self):
        pass

    def __exit__(self, type, value, traceback):
        pass

    def read():
        pass

    def readline():
        pass


class Vsfs:
    def __init__(self, host, port, storage_manager='posix'):
        """Initialize Vsfs connection.
        """
        self.host = host
        self.port = port
        self.storage_manager = storage_manager
        #self.conn = vsfs_py.Vsfs(host, port)

    def connect(self):
        """Connect to master.
        """
        pass

    def disconnect(self):
        """Disconnect.
        """
        pass
