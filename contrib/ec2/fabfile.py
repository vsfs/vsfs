#!/usr/bin/env python
#
# Copyright 2014 (c) Lei Xu <eddyxu@gmail.com>
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

from fabric.api import task
import yaml
import logging
import vsfs_ec2 as ec2

vsfs = ec2.VsfsEC2()

logging.basicConfig(format='[%(asctime)s](%(levelname)s) %(message)s',
                    level=logging.INFO)


@task
def help(name=''):
    """Print full information of the function. (name='task')
    """
    if not name:
        print("Use: 'fab help:func_name' for detailed help for each task.")
    else:
        print(globals()[name].__doc__)


@task
def price_history(instance='m1.small', n=10):
    """Print out the recent price history (instance='m1.small',n=10).
    """
    vsfs.get_spot_price_history(instance_type=instance, num_prices=n)


@task
def spot_requests():
    """Prints all active spot instance requests.
    """
    vsfs.get_all_spot_requests()


@task
def instances(state='running'):
    """Prints the information of instance.
    """
    vsfs.get_all_instances(state)


@task
def image_list():
    """Prints all images.
    """
    vsfs.print_all_images()


@task
def image_create(price=0.01, spot='yes', revision='HEAD', branch='master'):
    """Creates an VSFS image using Spot Instance (price=0.01,spot=yes/no).

    Options:
    @param spot set to 'yes' to use spot instance, set to 'no' to use on-demand
                instance. default: 'yes'
    @param price the bid price for spot instance. default: 0.01
    @param branch git branch of the vsfs source.
    @param revision the git revsion of the vsfs source.
    """
    if spot == 'yes':
        vsfs.create_image_spot(price)
    else:
        vsfs.create_image()


@task
def image_delete(image_id):
    """Deleted a stored image with the given ID.
    """
    vsfs.delete_image(image_id)


@task
def security_group_list():
    """List out all security groups.
    """
    vsfs.print_security_groups()


@task
def cluster_start(ami, nmaster, nindexd, nclient, yaml='example.yaml'):
    """Starts a cluster (ami='', nmaster=0, nindexd=0, nclient=0, \
yaml='example.yaml')

    Configuration of cluster is defined in 'example.yaml'
    """
    num_masters = int(nmaster)
    num_indexd = int(nindexd)
    num_client = int(nclient)
    vsfs.start_cluster(ami, num_masters, num_indexd, num_client,
                       conf_yaml=yaml)


@task
def vpc_list():
    """Prints all available VPC and its detailed information.
    """
    vsfs.print_vpcs()


@task
def vpc_create():
    """Creates a 10.0.0.0/22 virtual private cluster (VPC).
    """
    vsfs.create_vpc()


@task
def vpc_clear():
    """Removes all virtual private clusters.
    """
    vsfs.remove_vpcs()


@task
def list_x509_certifications():
    print vsfs.locate_x509_certifications()


@task
def s3_space():
    """Calculate s3 space consumption.
    """
    vsfs.print_s3_space()


@task
def volume_list():
    """List all volumes
    """
    vsfs.print_volumes()


@task
def volume_create(ami, price, volsize):
    """Creates a new EBS volume and format it (param: ami, price, volsize)
    """
    vsfs.create_volume_spot(ami, price, volsize)


@task
def elastic_ip_list():
    """List all elastic ips.
    """
    vsfs.print_elastic_ips()


@task
def test_run():
    """Start cluster on active instances.
    """
    confs = {}
    with open('test.yaml') as fobj:
        confs = yaml.load(fobj.read())

    vsfs.start_test_cluster(confs)
