#!/usr/bin/env python2.7
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

import os
basedir = os.path.dirname(__file__)
topdir = os.path.join(basedir, '..', '..', '..', '..')
import sys
sys.path.append(os.path.join(topdir, 'vsfs/fuse/testing'))
from fuse_tests import FuseTestBase
from subprocess import call, check_call, check_output
import os
import time
import unittest

VSFS = os.path.join(topdir, 'vsfs/ui/cli/vsfs')

class CommandsTest(FuseTestBase):
    """Integration test for commands
    """
    def setUp(self):
        super(CommandsTest, self).setUp()
        check_call('%s/../mount.vsfs -b %s -H localhost %s' %
                   (self.script_dir, self.base_dir, self.mount_dir),
                   shell=True)
        time.sleep(1)

    def test_list(self):
        testdir = os.path.join(self.mount_dir, 'test')
        self.assertEqual(0, call('mkdir -p %s' % testdir, shell=True))
        self.assertEqual(0, call('mkdir -p %s/foo' % testdir, shell=True))
        self.assertEqual(0, call(
            '{} index create -t btree -k float /test energy'.format(VSFS),
            shell=True))
        self.assertEqual(0, call(
            '{} index create -t btree -k int32 /test/foo fooindex'.format(VSFS),
            shell=True))
        list_output = check_output('{} index list -r /test'.format(VSFS),
                                   shell=True)
        self.assertEqual("""Indices on: /test/foo
  - fooindex (btree, int32)
Indices on: /test
  - energy (btree, float)\n""", list_output)

if __name__ == '__main__':
    unittest.main()
