#!/usr/bin/env python
#
# Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
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

"""Function tests for FUSE.
"""

import fcntl
import io
import os
import shutil
from stat import *
import struct
import subprocess
import termios
import tempfile
import time
import unittest

CWD = os.path.dirname(__file__)
MASTERD = os.path.join(CWD, os.pardir, os.pardir, 'masterd', 'masterd')
INDEXD = os.path.join(CWD, os.pardir, os.pardir, 'indexd', 'indexd')
VSFSUTIL = os.path.join(CWD, os.pardir, os.pardir, 'client', 'vsfs')


class FuseTestBase(unittest.TestCase):

    def setUp(self):
        self.script_dir = os.path.dirname(__file__)
        self.base_dir = tempfile.mkdtemp()

        # Starts the VSFS cluster.
        self.masterd_dir = os.path.join(self.base_dir, 'masterd')
        os.makedirs(self.masterd_dir)
        self.index_server_dir1 = os.path.join(self.base_dir, '1')
        os.makedirs(self.index_server_dir1)
        self.index_server_dir2 = os.path.join(self.base_dir, '2')
        os.makedirs(self.index_server_dir2)
        self.mount_dir = tempfile.mkdtemp()

        self.masterd_proc = subprocess.Popen(
            [MASTERD, '-primary', '-dir', self.masterd_dir])
        time.sleep(1)
        self.index_server_proc1 = subprocess.Popen(
            [INDEXD, '-datadir', self.index_server_dir1,
             '-master_addr', 'localhost', '-port', '10011'])
        self.index_server_proc2 = subprocess.Popen(
            [INDEXD, '-datadir', self.index_server_dir2,
             '-master_addr', 'localhost', '-port', '10012'])
        time.sleep(0.5)

        # TODO(lxu): scans the output of each server to determine whether it is
        # fully started.
        time.sleep(5)

    def tearDown(self):
        os.system('fusermount -u %s' % self.mount_dir)
        self.index_server_proc1.terminate()
        self.index_server_proc1.wait()
        self.index_server_proc2.terminate()
        self.index_server_proc2.wait()
        self.masterd_proc.terminate()
        #self.masterd_proc.kill()
        self.masterd_proc.wait()
        shutil.rmtree(self.base_dir)
        shutil.rmtree(self.mount_dir)

    def run_filebench(self, mntdir, workload, runtime):
        """Run filbench on mntdir
        @param mntdir mount point of VSFS fuse
        @param workload the filebench workload to run
        @param runtime the seconds to run the benchmark
        """
        conf = """load %s
set $dir=%s
run %d\n""" % (workload, mntdir, runtime)
        conf_file = tempfile.NamedTemporaryFile()
        conf_file.write(conf)
        conf_file.flush()
        p = subprocess.Popen('filebench -f %s' % conf_file.name,
                             stderr=subprocess.STDOUT, stdout=subprocess.PIPE,
                             shell=True)
        for line in p.stdout:
            if line.find('NO VALID RESULTS') >= 0:
                raise RuntimeError, "Failed filebench test."


class FusePosixStorageTest(FuseTestBase):
    """System tests on the FUSE-based VSFS.
    """
    def setUp(self):
        super(FusePosixStorageTest, self).setUp()
        subprocess.check_call('%s/../mount.vsfs -b %s -H localhost %s' %
                              (self.script_dir, self.base_dir, self.mount_dir),
                              shell=True)
        time.sleep(1)
        # Check we are running on a successfully mounted FUSE system.
        self.assertTrue(os.path.ismount(self.mount_dir))

    def test_mkdirs(self):
        self.assertEqual(0, os.system('mkdir -p %s/a/b/c' % self.mount_dir))
        self.assertTrue(os.path.exists('%s/a/b/c' % self.mount_dir))
        for i in range(10):
            os.makedirs('%s/a/b/c/d%d' % (self.mount_dir, i))
        subdirs = os.listdir('%s/a/b/c' % self.mount_dir)
        self.assertEqual(10, len(subdirs))

    def test_chmod(self):
        self.assertEqual(0, os.system('mkdir -p %s/a/b/c' % self.mount_dir))
        os.chmod('%s/a/b/c' % self.mount_dir, 0777)
        statinfo = os.stat('%s/a/b/c' % self.mount_dir)
        #self.assertEqual(0775 | stat.S_IFDIR, statinfo.st_mode)

    def test_create_file(self):
        self.assertEqual(0, os.system('touch %s/abc.txt' % self.mount_dir))
        self.assertTrue(os.path.exists('%s/abc.txt' % self.base_dir))

        self.assertEqual(0, os.system('echo 123 > %s/123.txt' %
                                      self.mount_dir))
        with open('%s/123.txt' % self.mount_dir) as fobj:
            content = fobj.read()
            self.assertEqual('123', content.strip())

    def test_remove_file(self):
        for i in range(10):  # Create 10 files first.
            with open('%s/%d.txt' % (self.mount_dir, i), 'w') as fobj:
                fobj.write('%s\n' % i)
            self.assertTrue(os.path.exists('%s/%d.txt' % (self.base_dir, i)))

        for i in range(10):
            os.remove('%s/%d.txt' % (self.mount_dir, i))
            self.assertFalse(os.path.exists('%s/%d.txt' % (self.base_dir, i)))


    def test_filebench_varmail(self):
        self.run_filebench(self.mount_dir, 'varmail', 10)

#    def _index_file(self, name, path, key):
#        """Insert a record into VSFS.
#        @param name the name of index
#        @param path the absolute path of the file.
#        @param key the key to inserted
#        """
#        vsfs_path = os.path.relpath(path, self.mount_dir)
#        if vsfs_path[0] != '/':
#            vsfs_path = '/' + vsfs_path
#        cmd = '%s index --name %s %s %s' % (VSFSUTIL, name,
#                                            vsfs_path, str(key))
#        return subprocess.call(cmd, shell=True)
#
#    def test_index_and_search_files(self):
#        os.makedirs('%s/energy' % self.mount_dir)
#
#        # Creates index
#        cmd = '%s index --create --name energy --type btree ' \
#              '--key uint64 %s' % (VSFSUTIL, '/energy')
#        self.assertEqual(0, subprocess.call(cmd, shell=True))
#
#        for i in range(100):  # creates 100 files.
#            test_file = '%s/energy/file-%d.txt' % (self.mount_dir, i)
#            with open(test_file, 'w') as fobj:
#                fobj.write('%d\n' % i)
#            self.assertEqual(0, self._index_file('energy', test_file, i))
#
#        expected_files = set(['#energy#file-%d.txt' % x for x in
#                              range(6, 100)])
#        files = os.listdir('%s/energy/?energy>5/' % self.mount_dir)
#        self.assertEqual(expected_files, set(files))
#
#        with open('%s/energy/?energy>5/#energy#file-20.txt' % self.mount_dir) \
#                as fobj:
#            self.assertEqual('20', fobj.read().strip())

class FuseObjectStorageTest(FuseTestBase):
    def setUp(self):
        super(FuseObjectStorageTest, self).setUp()
        subprocess.check_call(
            '%s/../mount.vsfs -b %s -s object -w 32 -H localhost %s' %
            (self.script_dir, self.base_dir, self.mount_dir),
            shell=True)
        time.sleep(1)
        # Check we are running on a successfully mounted FUSE system.
        self.assertTrue(os.path.ismount(self.mount_dir))

    def test_mkdirs(self):
        pass

    def test_filebench_varmail(self):
        self.run_filebench(self.mount_dir, 'varmail', 10)

if __name__ == '__main__':
    unittest.main()
