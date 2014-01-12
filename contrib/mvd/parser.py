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

"""Import MVD data into VSFS
"""

from __future__ import print_function
import argparse
import os

def extract_features(mol_file):
    features = {}
    with open(mol_file) as fobj:
        for line in fobj:
            if line.startswith('# Energy: '):
                fields = line.split()
                features['energy'] = float(fields[2])
    return features


def find_vsfs_prefix(path):
    """Find the prefix path where the vsfs mounts.
    """
    prefix = ''
    path = os.path.abspath(path)
    while path != '/':
        if not os.path.ismount(path):
            # print(path)
            path = os.path.dirname(path)
            continue
        with open('/proc/mounts') as fobj:
            for line in fobj:
                fields = line.split()
                if fields[1] == path:
                    if fields[2] != 'fuse.mount.vsfs':
                        print('Data file is not in VSFS: {}'.format(fields[3]))
                        return ''
                    return path
            break

    return prefix


def main():
    """Main function of ....
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('file', nargs='+', help='MOL2 files')
    args = parser.parse_args()

    prefix = find_vsfs_prefix(args.file[0])
    if not prefix:
        print("Files are not in VSFS?")
        return -1

    for mol_file in args.file:
        abslute_path = os.path.abspath(mol_file)
        features = extract_features(abslute_path)
        print(features, '/' + os.path.relpath(abslute_path, prefix))

if __name__ == '__main__':
    main()
