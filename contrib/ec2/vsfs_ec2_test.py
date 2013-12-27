#!/usr/bin/env python
#
# Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Unit tests for vsfs ec2
"""

import unittest
from multiprocessing import Process
import boto
import moto
from moto import mock_ec2
import vsfs_ec2 as ec2


KEY_FILE = '~/.ssh/ec2.pem'

@mock_ec2
def test_ec2_instance_wait():
    vsfs = ec2.VsfsEC2()
    #cluster = ec2.Cluster(vsfs)
    #cluster_p = Process(target=cluster.start_spot, args=({'ami': '124345',
    #                    'subnet': 'subnet-1234', 'security_group_ids': ['1']},))

    conn = boto.connect_ec2()
    conn.request_spot_instances(
            price=0.5, image_id='ami-abcd1234',
        )

    requests = conn.get_all_spot_instance_requests()
    print(type(requests))
    print(type(requests), dir(requests))
    requests.should.have.length_of(1)
