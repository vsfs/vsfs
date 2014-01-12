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

import sys
sys.path.append('../../../vsfs/fuse/testing')
from fuse_tests import FuseTestBase
from subprocess import call, check_call
import os
import time
import unittest

VSFS = '../../../vsfs/ui/cli/vsfs'

class MVDFuseTest(FuseTestBase):
    def setUp(self):
        super(MVDFuseTest, self).setUp()
        check_call('%s/../mount.vsfs -b %s -H localhost %s' %
                   (self.script_dir, self.base_dir, self.mount_dir),
                   shell=True)
        time.sleep(1)

    def test_import(self):
        testdir = os.path.join(self.mount_dir, 'test')
        self.assertEqual(0, call('mkdir -p %s' % testdir, shell=True))
        self.assertEqual(
            0, call('%s index create -t btree -k float /test energy' % VSFS,
                    shell=True))
        self.assertEqual(
            0, call('%s index list /test' % VSFS, shell=True))
        self.assertEqual(
            0, call('cp *.mol2 %s' % testdir, shell=True))
        self.assertEqual(
            0, call('../parser.py %s/*' % testdir, shell=True))


if __name__ == '__main__':
    unittest.main()
