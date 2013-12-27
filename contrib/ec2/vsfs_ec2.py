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

"""
The basic building block of running VSFS and benchmarks on EC2
"""

from __future__ import print_function
from datetime import datetime
from fabric.api import cd, env, sudo, settings, put, run, execute, parallel
from fabric.colors import yellow
import boto.ec2
import boto.vpc
import fabric.exceptions
import yaml
import glob
import logging
import os
import time

INSTALL_SCRIPT = \
    'https://raw.github.com/vsfs/vsfs-devtools/master/install-devbox.sh'
VSFS_GIT_URL = 'https://github.com/vsfs/vsfs.git'


class Volume(object):
    """Encaputure operations for EBS volume.
    """
    def __init__(self, ec2, volume=None, volume_id=None):
        if not volume and not volume_id:
            raise RuntimeError("You must specify one of Volume and VolumeId")
        self.ec2 = ec2
        if volume:
            self.vol = volume
        else:
            volumes = self.ec2.get_all_volumes(volume_ids=[volume_id])
            assert len(volumes) == 1
            self.vol = volumes[0]

    def wait(self, state, sleep_time=10):
        while True:
            logging.info('The volume (%s) state is %s..', self.vol.id,
                         self.vol.attachment_state())
            if self.vol.attachment_state() == state:
                break
            print(yellow('Sleep %d seconds...' % sleep_time))
            time.sleep(sleep_time)
            self.vol.update()

    def detach(self, force=False):
        if self.attached():
            self.vol.detach(force=force)
            self.wait(None)
        return True

    def status(self):
        return self.vol.state()

    def attached(self):
        return self.vol.attachment_state() == 'attached'


class SpotRequest(object):
    """A Spot Instance request
    """
    def __init__(self, ec2, ami, count=1, subnet=None,
                 security_group_ids=None):
        """Initialize a spot instance request with ec2 connection and AMI.

        @param conn an established EC2 connection.
        @param ami an AMI id.

        Optional parameters:
        @param count the number of instances requested.
        """
        self.ec2 = ec2
        self.conn = self.ec2.conn
        self.ami = ami
        self.security_groups = self.ec2.security_groups
        self.key_name = self.ec2.key_name
        self.count = count
        self.requests = None
        self.instances = []
        # For VPC
        self.subnet = subnet
        self.security_group_ids = security_group_ids

        if self.subnet and not self.security_group_ids:
            message = 'Issuing an Spot Instance Request in VPC requires ' \
                      'security_group_ids being set.'
            logging.fatal(message)
            raise RuntimeError(message)

    @staticmethod
    def to_string(request):
        """Format a boto.ec2.SpotInstanceRequest to string.
        """
        return 'Spot instance request: request_id={}, price={}, ' \
               'instance_id={}, type={}, state={}'.format(
                   request.id, request.price, request.instance_id,
                   request.type, request.state)

    @staticmethod
    def print_all_requests(conn):
        """Prints infomation of all requests.

        @param conn an established boto.EC2Connection.
        """
        logging.info('Querying for all spot instance requests..')
        requests = conn.get_all_spot_instance_requests()
        for request in requests:
            if request.state == 'cancelled':
                continue
            print(SpotRequest.to_string(request))

    def __str__(self):
        """Return string representation of a spot request.
        """
        return SpotRequest.to_string(self.request)

    def __del__(self):
        """Cancel all left-over spot instance requests.
        """
        self.cancel()

    def request(self, price, instance_type='m1.small'):
        """Issue an new Spot Instance request.
        """
        if self.subnet:  # Request within VPC
            self.requests = self.conn.request_spot_instances(
                price, self.ami, count=self.count, instance_type=instance_type,
                subnet_id=self.subnet,
                security_group_ids=self.security_group_ids,
                key_name=self.key_name)
        else:  # Normal Spot Requests.
            self.requests = self.conn.request_spot_instances(
                price, self.ami, count=self.count, instance_type=instance_type,
                security_groups=self.security_groups,
                key_name=self.key_name)

    def cancel(self):
        """Cancels the request.
        """
        if self.requests:
            for request in self.requests:
                request.cancel()

    def ready(self, no_wait=False, sleep_time=30):
        """Returns true when the instances are ready.
        """
        if not self.request:
            logging.error('No request has been issued.')
            return False
        try:
            while True:
                requests = self.conn.get_all_spot_instance_requests(
                    request_ids=[r.id for r in self.requests])
                if len(requests) == 0:
                    print(yellow('Sleep %d seconds...' % sleep_time))
                    print(yellow('Press Ctrl-C to cancel...'))
                    if no_wait:
                        return False
                    time.sleep(sleep_time)
                    continue
                succ_instance_ids = []  # success instances
                for request in requests:
                    logging.info(SpotRequest.to_string(request))
                    if request.state == 'cancelled':
                        logging.info('The spot request has been canceled.')
                        logging.info(SpotRequest.to_string(request))
                        return False
                    elif request.state == 'active':
                        logging.info('Got instances, lets roll..: '
                                     'instance_id=%s', request.instance_id)
                        succ_instance_ids.append(request.instance_id)
                # All instance requests are full filled.
                if len(succ_instance_ids) == len(self.requests):
                    logging.info('All instances are full filled.')
                    self.instances = []
                    for reservation in self.conn.get_all_instances(
                            instance_ids=succ_instance_ids):
                        self.instances.extend(reservation.instances)
                    return True
                if no_wait:
                    return False
                logging.info('Sleep for %d seconds...', sleep_time)
                time.sleep(sleep_time)
        except KeyboardInterrupt:
            print(yellow('Ctrl-C received, canceling the spot request.'))
            self.cancel()


class EC2Instance(object):
    """An EC2 instance.
    """
    USER = 'ubuntu'

    def __init__(self, ec2_instance, key_file):
        self.instance = ec2_instance
        self.key_file = key_file
        self.retry = 10
        self.sleep_time = 30

    @staticmethod
    def wait_running(instance, sleep_time=30):
        """
        """
        wait_for_boot = 1
        while True:
            instance.update()
            if instance.state == 'terminated':
                return False
            if instance.state == 'pending':
                print(yellow('Waitting for booting...{} times'.format(
                             wait_for_boot)))
                wait_for_boot += 1
                time.sleep(sleep_time)
                continue
            if instance.state == 'running':
                logging.info('The instance is ready...')
                return True

    def wait(self):
        """Wait until the instance is running.
        """
        EC2Instance.wait_running(self.instance)

    def execute(self, commands):
        """
        @param commands a list of [(cmd, 'param'), (cmd, 'param'),...]
        """
        host_string = self.instance.public_dns_name
        if not host_string:
            host_string = self.instance.private_dns_name
        retry = self.retry
        while retry > 0:
            try:
                with settings(host_string=host_string):
                    for cmd in commands:
                        command = cmd[0]
                        param = cmd[1]
                        logging.info('Execute %s(%s)', cmd[0].__name__, param)
                        command(param, shell=True)
                    return True
            except fabric.exceptions.NetworkError:
                retry -= 1
                logging.warning('Could not connected to the VM, maybe it is '
                                'still starting?...')
                print(yellow('Sleep %d seconds...' % self.sleep_time))
                time.sleep(self.sleep_time)
        return False


class VsfsMasterDaemon(object):
    """Basic operations on master daemon.

    A small clusters of Master servers.
    """
    def __init__(self, hosts, basedir='/mnt/data'):
        self.hosts = hosts
        self.basedir = basedir
        self.masterd = os.path.join(self.basedir, 'vsfs/vsfs/masterd/masterd')
        self.data_dir = '/tmp/vsfs/data'
        self.log_data = '/tmp/vsfs/log'

    def start(self):
        """Starts this little cluster of master servers.
        """
        with settings(parallel=True, hosts=self.hosts):
            pass

    @staticmethod
    @parallel
    def kill():
        """Kill all masterd in parallel.
        """
        run('pkill masterd || true')

    def stop(self):
        with settings(hosts=self.hosts):
            execute(VsfsMasterDaemon.kill)


class Cluster(object):
    """Setup/Start/Stop a VSFS cluster on EC2.
    """
    USER = 'ubuntu'

    def __init__(self, ec2):
        """Initialize a VSFS Cluster on EC2.

        @param ec2 the VsfsEC2 instance.
        """
        self.ec2 = ec2
        self.conn = self.ec2.conn  # shared boto connection with VsfsEC2
        self.head_instance = None
        self.masterd_instances = []
        self.indexd_instances = []
        self.confs = None  # Cluster configuration dictionary.

    @staticmethod
    def create_security_group(ec2_conn, name='vsfs', **kwargs):
        """Creates security group for VSFS to open ports for components.
        @param ec2_conn an established boto connection.
        @param name the name of this security_group

        Optional parameters
        @param cidr customize the VPC CIDR, default 10.0.0.0/22
        """
        cidr = kwargs.get('cidr', '10.0.0.0/22')

        # TODO(eddyxu): use more strict mask.
        logging.info('Creating security group %s (%s)', name, cidr)

        vsfs_security_group = ec2_conn.create_security_group(
            name, 'Security Group for VSFS Cluster')
        vsfs_security_group.authorize('tcp', 22, 22, cidr)
        # NFS
        vsfs_security_group.authorize('tcp', 111, 111, cidr)
        vsfs_security_group.authorize('udp', 111, 111, cidr)
        vsfs_security_group.authorize('tcp', 2049, 2049, cidr)
        vsfs_security_group.authorize('udp', 2049, 2049, cidr)

        # VSFS
        # Masterd
        vsfs_security_group.authorize('tcp', 9876, 9876, cidr)
        # Indexd
        vsfs_security_group.authorize('tcp', 5555, 5555, cidr)

    def start(self):
        """Acquire enough EC2 instances and start the cluster.
        """
        pass

    def start_spot(self, confs):
        """Start VsfsEC2 Cluster using spot instances.

        @param confs a dictionary to configure the spot request(s) for
        a VSFS cluster.
        """
        self.confs = confs
        sleep_time = 20  # 20 seconds
        logging.info('Starting spot cluster...: %s', confs)
        head_request = SpotRequest(
            self.ec2, confs['ami'], subnet=confs['subnet'],
            security_group_ids=confs['security_group_ids'])
        head_request.request('0.01')
        if not head_request.ready():  # block until it is ready.
            logging.error('Failed to require head node.')
            return

        self.head_instance = head_request.instances[0]
        self.config_head()

        spot_requests = {}
        # Only test masterd and indexd so far.
        for server in ['master', 'indexd']:
            if confs[server]['count'] > 0:
                spot_requests[server] = SpotRequest(
                    self.ec2, confs['ami'], count=confs[server]['count'])
                spot_requests[server].request(confs[server]['price'])
        try:
            while True:
                total_ready = 0
                for server in spot_requests.keys():
                    spot_req = spot_requests[server]
                    if spot_req.ready(no_wait=True):
                        logging.info('Spot request for %s is ready', server)
                        total_ready += 1
                if total_ready < len(spot_requests):
                    logging.info('Wait for all spot requests to be ready...')
                    print(yellow('Sleep %d seconds...' % sleep_time))
                    time.sleep(sleep_time)
                else:
                    break
        except KeyboardInterrupt:
            logging.warning('Canceling spot instance requesting for cluster.')
            for spot_req in spot_requests.values():
                spot_req.cancel()
            return

        self.start_cluster()

    def config_head(self, volume_id=None):
        """Configure the head node.

        @param vol_id the ID of the EBS volume to store persistent data.
        """
        if not self.head_instance:
            logging.error("Head node instance is not ready.")
            return False

        # Mount Volume.
        if not volume_id:
            volume_id = self.confs['volume']
        vol = self.conn.get_all_volumes(volume_ids=[volume_id])
        if not vol:
            logging.error("The volume %s is not found.", volume_id)
            return False
        head_instance = EC2Instance(self.head_instance, self.ec2.aws_key_file)
        head_instance.wait()
        # In ubuntu, this device is renamed to '/dev/xvdc'
        vol[0].attach(self.head_instance.id, '/dev/sdc')
        while True:
            logging.info('The volume is %s..', vol[0].attachment_state())
            if vol[0].attachment_state() == 'attached':
                break
            print(yellow('Sleep %d seconds...' % 10))
            time.sleep(10)
            vol[0].update()

        with settings(warn_only=True,
                      host_string=self.head_instance.private_dns_name):
            sudo('mkdir -p /mnt/data')
            sudo('mount -t ext4 -o noatime /dev/xvdc /mnt/data')
            sudo("echo '/mnt/data 10.0.0.0/255.255.0.0"
                 "(async,no_root_squash,rw,no_subtree_check,fsid=0)'"
                 " > /etc/exports")
            #(sudo, "service rpcbind restart")
            sudo("exportfs -ra", pty=False)
            sudo("rpc.idmapd", pty=False)
            sudo("service nfs-kernel-server restart", pty=False)
        return True

    def start_cluster(self):
        """Configure the cluster and start VSFS cluster.

        @pre all instances have been started.

        Starting a cluster involves the following steps.
        - Set up head node.
          - Mount EBS volume.
          - (Optional) build a new version of VSFS on the EBS volume.
          - Export NFS mount point of the volue
        - Set up masterd
          - Mount NFS from head node.
          - Start 'masterd' and connect to the primary masterd.
             - Set output directory to the NFS mount point.
        - Set up indexd
          - Mount NFS from head node.
          - Start 'indexd' and connect to the master cluster.
             - Set output directory to the NFS mount point.
        - Issue clients
        """
        self.start_master_servers()
        self.start_index_servers()

    @staticmethod
    def _mount_nfs(url, mntpnt):
        """Parallel mount NFS server through SSH connection.

        @param url the URL of nfs server (e.g., 10.0.1.1:/home/data)
        @param mntpnt the mount point locally.
        """
        sudo('mkdir -p %s' % mntpnt, shell=True)
        sudo('mount -t nfs -o vers=3,proto=tcp %s %s' % (url, mntpnt),
             shell=True)

    def _get_hosts(self, instances, name):
        sleep_time = 10
        hosts = []
        for instance in instances:
            while True:
                if instance.state != 'pending':
                    logging.info('%s: %s is %s',
                                 name, instance.id, instance.state)
                    break
                else:
                    print(yellow('Sleep %d seconds...' % sleep_time))
                    time.sleep(sleep_time)
                    instance.update()
            hosts.append(instance.private_dns_name)
        return hosts

    def start_master_servers(self):
        """Start VSFS' master server.
        """
        hosts = self._get_hosts(self.masterd_instances, 'Master Server')
        # Mount NFS
        with settings(parallel=True, hosts=hosts):
            nfs_url = self.head_instance.private_dns_name + ":/mnt/data"
            mnt_point = "/mnt/data"
            logging.info('Mount NFS on %s', hosts)
            execute(Cluster._mount_nfs, nfs_url, mnt_point)

        # Primary node.
        with settings(host_string=hosts[0]):
            run('/mnt/data/vsfs/vsfs/masterd/masterd -primary '
                '-daemon -dir /tmp -log_dir /tmp', pty=False)
        # Slavary nodes.
        if len(hosts) > 1:
            with settings(hosts=hosts[1:]):
                run('/mnt/data/vsfs/vsfs/masterd/masterd -primary_host=%s'
                    ' -daemon -dir /tmp -log_dir /tmp' % hosts[0], pty=False)

    @staticmethod
    @parallel
    def _start_indexd(master_host):
        log_dir = '/tmp/vsfs/log'
        data_dir = '/tmp/vsfs/data'
        run('mkdir -p %s' % data_dir, shell=True)
        run('mkdir -p %s' % log_dir, shell=True)
        run('/mnt/data/vsfs/vsfs/indexd/indexd -master_addr %s '
            ' -daemon -log_dir %s -datadir %s -update_immediately' %
            (master_host, log_dir, data_dir))

    def start_index_servers(self):
        """Start all index servers.
        """
        hosts = self._get_hosts(self.indexd_instances, 'Index Server')
        # Mount NFS
        with settings(parallel=True, hosts=hosts):
            nfs_url = self.head_instance.private_dns_name + ":/mnt/data"
            mnt_point = "/mnt/data"
            logging.info('Mount NFS on %s', hosts)
            execute(Cluster._mount_nfs, nfs_url, mnt_point)
            execute(Cluster._start_indexd,
                    self.masterd_instances[0].private_dns_name)

    def stop(self):
        """Terminate all instances.
        """
        #TODO(eddyxu): terminate clients?
        logging.info('Stopping cluster...')
        if self.indexd_instances:
            self.conn.terminate_instances(instance_ids=self.indexd_instances)
        if self.masterd_instances:
            self.conn.terminate_instances(instance_ids=self.masterd_instances)
        logging.info('The cluster has stopped.')

    def addresses(self):
        """Getheres all components' ip addresses.

        The cluster must has been established.
        """
        results = {}
        results['masterd'] = []
        for master in self.masterd_instances:
            assert master.ip_address
            results['masterd'].append(master.ip_address)
        results['indexd'] = []
        for indexd in self.indexd_instances:
            assert indexd.ip_address
            results['indexd'].append(indexd.ip_address)

    @staticmethod
    @parallel
    def _umount_nfs(mntpnt):
        """Umount NFS client.
        """
        sudo('umount -f %s || true' % mntpnt)

    def gracefully_shutdown(self):
        """Gracefully kill all servers and umount data
        """
        logging.info('Gracefully shutting down VSFS services')
        # Kill clients
        # Kill indexds
        # Kill masterds
        hosts = [instance.private_dns_name for instance
                 in self.masterd_instances]
        with settings(hosts=hosts):
            execute(VsfsMasterDaemon.kill)

        # umount nfs servers
        hosts = [instance.private_dns_name for instance in
                 self.masterd_instances + self.indexd_instances]
        logging.info('Umounting NFS clients on %s...', hosts)
        with settings(hosts=hosts):
            execute(Cluster._umount_nfs, '/mnt/data')


# pylint: disable=R0904
class VsfsEC2(object):
    """Run VSFS on Amazon EC2

    It requires that users define AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY
    as environment variables.
    """
    EC2_REGION = 'us-east-1'
    EC2_AMI = 'ami-271a484e'  # Ubuntu 13.10 instance
    EC2_KEY_FILE = '~/.ssh/ec2.pem'
    EC2_SECURITY_GROUPS = ['quick-start-1']
    EC2_USER = 'ubuntu'
    EC2_INSTANCE_TYPE = 'm1.small'
    EC2_KEY_NAME = 'eddy'  # TODO(eddyxu): change later.

    def __init__(self, **kwargs):
        """Initialize a VSFS EC2 connection.

        'AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY' and 'AWS_KEY_FILE'
        environment variables must be set before initializing the VsfsEC2
        object.

        Keyword parameters
        @param region set the EC2 region, default 'us-east-1'
        @param ami set the ami to run VSFS, default 'ami-271a484e' (ubuntu
        13.10)
        @param user set the user to ssh login into vm, default 'ubuntu'
        @param security_groups set the security groups.
        @param key_name the key used to login the VM.
        """
        # Set running configuration.
        self.region = kwargs.get('region', self.EC2_REGION)
        self.ami = kwargs.get('ami', self.EC2_AMI)
        self.user = kwargs.get('user', self.EC2_USER)
        self.security_groups = kwargs.get('security_groups',
                                          self.EC2_SECURITY_GROUPS)
        self.key_name = kwargs.get('key_name', self.EC2_KEY_NAME)

        for env_var in ['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY',
                        'AWS_ACCOUNT_ID']:
            if not env_var in os.environ:
                logging.error(
                    'AWS environment variable not found: %s', env_var)
                raise RuntimeError('AWS environment variable not found.')

        self.aws_access_key_id = os.environ.get('AWS_ACCESS_KEY_ID')
        self.aws_secret_access_key = os.environ.get('AWS_SECRET_ACCESS_KEY')
        self.aws_account_id = os.environ.get('AWS_ACCOUNT_ID')
        self.aws_key_file = os.environ.get('AWS_KEY_FILE', self.EC2_KEY_FILE)

        env.user = self.user
        env.key_filename = self.aws_key_file

        if not os.path.exists(os.path.expanduser(self.aws_key_file)):
            logging.error('AWS key file does not exist: %s', self.aws_key_file)
            raise RuntimeError('Aws key file does not exist.')

        self.conn = boto.ec2.connect_to_region(
            self.region, aws_access_key_id=self.aws_access_key_id,
            aws_secret_access_key=self.aws_secret_access_key)
        if not self.conn:
            raise RuntimeError("Failed to initialize connection to EC2.")

        #print(self.conn.get_all_zones())
        #print(dir(self.conn.get_all_zones()[0]))
        #print(self.conn.get_all_zones()[0].name)
        # Connect to VPC
        region = None
        for reg in boto.ec2.regions():
            if reg.name == self.region:
                region = reg
        self.vpc = boto.vpc.VPCConnection(
            region=region,
            aws_access_key_id=self.aws_access_key_id,
            aws_secret_access_key=self.aws_secret_access_key)
        if not self.vpc:
            raise RuntimeError("Failed to connect to VPC")

    def get_spot_price_history(self, **kwargs):
        """List out the spot price history.

        @param num_prices only report the n most recent prices. Sets to 0 to
        return all prices.
        @param instance_type choose the instance type (default: m1.small)
        """
        instance_type = kwargs.get('instance_type', self.EC2_INSTANCE_TYPE)
        num_prices = kwargs.get('num_prices', 0)
        product = 'Linux/UNIX'
        #product = 'Linux/UNIX (Amazon VPC)'
        prices = self.conn.get_spot_price_history(
            instance_type=instance_type,
            product_description=product)
        print('Time\tInstance\tPrice($$)')
        if num_prices > 0:
            prices = prices[:num_prices]
        for price in prices[:30]:
            print('{}\t{}\t{}'.format(price.timestamp, price.instance_type,
                                      price.price))

    def _print_instance(self, instance):
        """Print the instance information.
        """
        print('Instance: id={}, type={}, public_dns={}, private_dns={}, '
              'state={}'.format(
                  instance.id, instance.instance_type,
                  instance.public_dns_name, instance.private_dns_name,
                  instance.state))

    def get_all_spot_requests(self):
        """Get all active spot instance requests.
        """
        SpotRequest.print_all_requests(self.conn)

    def get_all_instances(self, state='running'):
        """Prints infomation of all running instances.
        """
        logging.info('Getting all instances...')
        reservations = self.conn.get_all_instances()
        for reservation in reservations:
            for instance in reservation.instances:
                if state and instance.state == state:
                    self._print_instance(instance)
                elif not state:  # Print all instances
                    self._print_instance(instance)

    def print_all_images(self):
        for image in self.conn.get_all_images(owners=['self']):
            #print(dir(image))
            print("Image(%s, %s): %s" %
                  (image.name, image.architecture, image.id))

    def print_security_groups(self):
        """Prints out the information of security groups of the current
        account.
        """
        print(yellow('Security Groups'))
        for sg in self.conn.get_all_security_groups():
            print('Name:', sg.name)
            print('  Rules:', sg.rules)

    def print_s3_space(self):
        """Print the space consuption of S3.
        """
        s3 = boto.connect_s3(self.aws_access_key_id,
                             self.aws_secret_access_key)
        b = s3.get_bucket('vsfs')
        total_size = 0
        for key in b.list():
            total_size += key.size
        print('S3 bucket size: %d bytes' % total_size)

    def get_subnet(self):
        """Gets the subnet from the first VPC.
        """
        all_vpcs = self.vpc.get_all_vpcs()
        if not all_vpcs:
            return None
        vpc = all_vpcs[0]
        subnets = self.vpc.get_all_subnets(filters={'vpcId': vpc.id})
        if not subnets:
            return None
        return subnets[0]

    def create_image_spot(self, price, subnet=None):
        """Creates an AMI image on the spot instance (to save $$$).
        """
        logging.info(yellow("Request a Spot instance."))
        if not subnet:
            subnet = self.get_subnet()
            if not subnet:
                logging.error("Please create a virtual private cluster first.")
                return

        #logging.info('Create spot instance request on subnet: %s' % subnet.id)
        spot_request = SpotRequest(self, self.ami, count=1)
        spot_request.request(price)
        time.sleep(10)
        if not spot_request.ready():
            return
        self.provision_image(spot_request.instances[0].id)
        self.create_bundle(spot_request.instances[0].id)
        spot_request.instances[0].terminate()

    def create_image(self):
        """Creates a AMI image that contains a VSFS binary that is compiled
        against HEAD.
        """
        print(yellow("Create a new instance."))
        self.conn.run_instances(self.ami, instance_type='m1.small',
                                security_groups=self.security_groups,
                                key_name=self.key_name)
        reservations = self.conn.get_all_instances()
        instance_id = None
        for reservation in reservations:
            instances = reservation.instances
            if len(instances) != 1 or instances[0].state == 'terminated':
                logging.info('Skip reservation: %s', reservation)
                continue
            instance_id = instances[0].id
            break
        if not instance_id:
            logging.error('No active instance found.')
            return -1
        self.provision_image(instance_id)
        self.create_bundle(instance_id)

    def delete_image(self, ami):
        """Deletes an image created by the current user.
        """
        logging.info("Deleting an image: %s", ami)
        self.conn.deregister_image(ami, delete_snapshot=True)

    @staticmethod
    def locate_x509_certifications(basedir='~/.ec2'):
        """Locate the pk-*.pem and cert-*.pem file
        """
        ret = {}
        logging.info('Locating X.509 certification (i.e. '
                     'pk-***.pem and cert-***.pem)')
        basedir = os.path.expanduser(basedir)
        if os.path.exists(basedir):
            files = glob.glob(os.path.join(basedir, 'pk-*.pem'))
            if len(files) == 1:
                logging.info('Found %s', files[0])
                ret['pk'] = files[0]
            files = glob.glob(os.path.join(basedir, 'cert-*.pem'))
            if len(files) == 1:
                logging.info('Found %s', files[0])
                ret['cert'] = files[0]
        if len(ret) != 2:
            logging.warning('Could not find X.509 Certification')
        return ret

    def create_bundle(self, instance_id):
        """Create a bundle (image) from a provisioned instance.
        """
        cert_files = self.locate_x509_certifications()
        if len(cert_files) != 2:
            return

        logging.info('Get reservations for instance: %s', instance_id)
        reservations = self.conn.get_all_instances(
            filters={'instance-id': instance_id})
        if len(reservations) != 1:
            logging.error('Wrong number of reservations: %s',
                          reservations)
            assert len(reservations) == 1
        instance = reservations[0].instances[0]
        assert instance.state == 'running'

        env.host_string = instance.ip_address
        with settings(user='ubuntu', key_filename=self.aws_key_file):
            sudo('echo "deb http://us-east-1.ec2.archive.ubuntu.com/ubuntu/ '
                 'saucy multiverse" > /etc/apt/sources.list.d/ec2.list')
            sudo('apt-get -y -qq update')
            sudo('apt-get install -y -qq ec2-ami-tools')
            # Setup EC2 AMI tools.
            sudo('mkdir /tmp/cert && chmod 777 /tmp/cert')
            put(self.aws_key_file, '/tmp/cert/key.pem')
            put(cert_files['pk'],
                os.path.join('/tmp/cert', os.path.basename(cert_files['pk'])))
            put(cert_files['cert'],
                os.path.join('/tmp/cert',
                             os.path.basename(cert_files['cert'])))
            sudo('ec2-bundle-vol -e /tmp/cert -k %s -c %s -u %s -r x86_64' %
                 ('/tmp/cert/' + os.path.basename(cert_files['pk']),
                  '/tmp/cert/' + os.path.basename(cert_files['cert']),
                  self.aws_account_id))

            now = datetime.now()
            timestamp = now.strftime('%Y-%m-%d-%H-%M')
            image_bucket = 'vsfs/images/image-' + timestamp
            sudo('ec2-upload-bundle -b %s -m /tmp/image.manifest.xml '
                 '-a %s -s %s' % (image_bucket,
                                  self.aws_access_key_id,
                                  self.aws_secret_access_key))
            self.conn.register_image(
                'vsfs-box-' + timestamp,
                description='ubuntu server image for vsfs test',
                image_location=image_bucket + '/image.manifest.xml')

    def provision_image(self, instance_id=None):
        """Install the packages and build VSFS binary.

        @param instance_id the ID of a running EC2 instance.
        @param revision the git revision of VSFS repository to checkout,
        default is head.
        """
        if not instance_id:
            reservations = self.conn.get_all_instances()
        else:
            reservations = self.conn.get_all_instances(
                filters={'instance-id': instance_id})
        for reservation in reservations:
            instances = reservation.instances
            if len(instances) != 1 or instances[0].state == 'terminated':
                print('Skip reservation: ', reservation)
                continue
            instance = instances[0]
            wait_for_boot = 1
            BOOT_SLEEP_TIME = 20  # check booting status for each 10 seconds.
            while instance.state == 'pending':
                print(yellow('Waitting for booting...{} times'.format(
                    wait_for_boot)))
                time.sleep(BOOT_SLEEP_TIME)
                wait_for_boot += 1
                instance.update()
            print(instance, instance.state, instance.ip_address)

            # Uses SSH to login into VM to provision it with dependencies and
            # build vsfs binary.
            env.host_string = "%s" % instance.ip_address
            retry = 10
            while retry > 0:
                try:
                    with settings(user='ubuntu',
                                  key_filename=self.aws_key_file):
                        sudo('apt-get -qq update')
                        sudo('apt-get install -y -qq wget libopenmpi1.6-dev '
                             'nfs-common nfs-kernel-server')
                        sudo('wget %s -O- | sh' % INSTALL_SCRIPT, shell=True)
                        sudo('echo "/usr/local/lib" > '
                             '/etc/ld.so.conf.d/vsfs.conf')
                        sudo('ldconfig')

                    break
                except fabric.exceptions.NetworkError:
                    retry -= 1
                    logging.warning(
                        'Could not connecte to the VM, maybe the VM '
                        'is still starting?...sleep 30 seconds...')
                    time.sleep(30)

    def terminate(self, instance_id):
        """Terminate an instance.
        """
        self.conn.terminate_instances(instance_ids=[instance_id])

    def print_vpcs(self):
        """Prints VPC configurations nicely.
        """
        for vpc in self.vpc.get_all_vpcs():
            print('Virtual private cluster (VPC) ID:', vpc.id)
            print(' CIDR:', vpc.cidr_block)
            print(' Subnets:')
            for subnet in self.vpc.get_all_subnets(filters={'vpcId': vpc.id}):
                print('  id:', subnet.id)
                print('  state:', subnet.state)
                print('  cidr:', subnet.cidr_block)
                print('  available ip addresses:',
                      subnet.available_ip_address_count)
            print(' Gateway:')
            for gateway in self.vpc.get_all_internet_gateways(
                    filters={'attachment.vpc-id': vpc.id}):
                #print(dir(gateway))
                print('  id:', gateway.id)
                print('  connection:', gateway.connection)
                #print('  item:', gateway.item)
            print(' Route table:')
            for route_table in self.vpc.get_all_route_tables(
                    filters={'vpc-id': vpc.id}):
                #print(dir(route_table))
                print('  id:', route_table.id)
                print('  routes:', route_table.routes)
                print('  associations:')
                for asso in route_table.associations:
                    #print(dir(asso))
                    print('   * id:', asso.id)
                    print('   * subnet:', asso.subnet_id)

    def create_vpc(self):
        """Creates an VPC to run VSFS cluster.
        """
        vpc = self.vpc.create_vpc('10.0.0.0/22')
        while vpc.state != 'available':
            logging.info('VPC state is pending...sleep 10 seconds...')
            time.sleep(10)
            vpc.update()

        self.vpc.create_subnet(vpc.id, '10.0.1.0/24')
        self.vpc.create_subnet(vpc.id, '10.0.0.0/24')
        gateway = self.vpc.create_internet_gateway()
        self.vpc.attach_internet_gateway(gateway.id, vpc.id)

    def remove_vpcs(self, vpc_id=None):
        """Remove one or all VPCs
        """
        def _delete_vpc(vpc_id):
            for subnet in self.vpc.get_all_subnets(filters={'vpcId': vpc_id}):
                self.vpc.delete_subnet(subnet.id)
            for gateway in self.vpc.get_all_internet_gateways(
                    filters=[('tag:Name', vpc_id)]):
                self.vpc.delete_internet_gateway(gateway.id)

        vpcs = None
        if not vpc_id:
            vpcs = self.vpc.get_all_vpcs()
        else:
            vpcs = self.vpc.get_all_vpcs(filters={'vpcId': vpc_id})
        for vpc in vpcs:
            _delete_vpc(vpc.id)
            vpc.delete()

    def print_volumes(self):
        """Prints all EBS volumes
        """
        all_vols = self.conn.get_all_volumes()
        print("All EBS volumes:")
        for vol in all_vols:
            print("  Volume(%s): %d GiB, %s, %s" %
                  (vol.id, vol.size, vol.status, vol.zone))

    def check_volume_availability(self, vol_id):
        """Check the availablity of an EBS volume.
        @param vol_id the id of the target volume.
        """
        logging.info('Check volume availability %s', vol_id)
        all_vols = self.conn.get_all_volumes(volume_ids=[vol_id])
        if not all_vols:
            logging.warning('The volume does not exist: %s', vol_id)
            return False
        volume = all_vols[0]
        if volume.status != 'available':
            logging.warning('The volume is not available, its status: %s',
                            volume.status)
            return False
        return True

    def create_volume_spot(self, ami, price, volsize):
        """Creates an data volume using a spot instance.

        The data volume contains the source code, compiled binary of vsfs,
        as well as some directories to store experiemntal data.
        """
        spot_request = SpotRequest(self, ami)
        spot_request.request(price)
        if not spot_request.ready():
            logging.error(
                "Failed to create spot instance to create volume: %s",
                spot_request)
            return False
        EC2Instance.wait_running(spot_request.instances[0])
        try:
            self.create_volume(spot_request.instances[0], volsize)
        except Exception as e:
            logging.error('Exception raised: %s, shutting down instance.', e)
            spot_request.instances[0].terminate()

    def create_volume(self, instance, volsize):
        """Creates a volume and attach to the instance.
        """
        volume_size = volsize  # GB, should be configured later.
        # Find the zone where the instance located.
        zone = None
        for z in self.conn.get_all_zones():
            if z.name == instance.placement:
                zone = z
                break
        assert zone
        ebs_volume = self.conn.create_volume(volume_size, zone)
        while ebs_volume.volume_state() != 'available':
            print(yellow('Waitting for EBS volume available...'
                         'sleep 10 seconds...'))
            time.sleep(10)
            ebs_volume.update()
        logging.info('EBS volume is ready, attaching to instance: %s',
                     instance.id)
        # In ubuntu, this device is renamed to '/dev/xvdc'
        ebs_volume.attach(instance.id, '/dev/sdc')

        # build vsfs binary.
        logging.info('Build data volume on %s', instance.public_dns_name)
        env.host_string = instance.ip_address
        with settings(user='ubuntu', key_filename=self.aws_key_file):
            logging.info('Building vsfs binary.')
            sudo('mkdir -p /mnt/data', shell=True)
            sudo('mkfs.ext4 /dev/xvdc', shell=True)
            sudo('mount -t ext4 -o noatime /dev/xvdc /mnt/data')
            sudo('chown -R ubuntu:ubuntu /mnt/data')
            with cd('/mnt/data'):
                run('git clone {}'.format(VSFS_GIT_URL), shell=True)
                with cd('/mnt/data/vsfs'):
                    run('./bootstrap', shell=True)
                    run('./configure', shell=True)
                    run('make', shell=True)
            sudo('umount /mnt/data')
        logging.info('Detaching EBS volume...')
        self.conn.detach_volume(ebs_volume.id)
        logging.info('Terminate instance...')
        instance.terminate()
        logging.info('Successfully create an data volume for test')

    def print_elastic_ips(self):
        """Print elastic ips
        """
        print('Elastic IPs:')
        for ip in self.conn.get_all_addresses():
            #print(dir(ip))
            print('{}: assication: {}, instance: {}, private: {}, public: {}'
                  .format(ip, ip.association_id, ip.instance_id,
                          ip.private_ip_address, ip.public_ip))

    def start_test_cluster(self, confs):
        """
        """
        self.check_volume_availability(confs['volume'])
        cluster = Cluster(self)
        print(confs)
        reservations = self.conn.get_all_instances(
            instance_ids=confs['instances'])
        cluster.head_instance = reservations[0].instances[0]
        cluster.masterd_instances.append(reservations[1].instances[0])
        cluster.indexd_instances.append(reservations[2].instances[0])
        cluster.gracefully_shutdown()
        logging.info('Head node is: %s, %s', cluster.head_instance,
                     cluster.head_instance.private_dns_name)
        volume = Volume(self.conn, volume_id=confs['volume'])
        if volume.attached():
            with settings(host_string=cluster.head_instance.private_dns_name):
                sudo('service nfs-kernel-server stop', pty=False)
                sudo('umount /mnt/data')
            volume.detach()

        cluster.config_head(confs['volume'])
        cluster.start_master_servers()
        cluster.start_index_servers()

    def start_cluster(self, ami, num_masterd, num_indexd, num_clients,
                      conf_yaml='example.yaml'):
        """Starts a Cluster with different number of masters.
        """
        confs = {}
        with open(conf_yaml) as fobj:
            confs = yaml.load(fobj.read())
        confs['ami'] = ami
        confs['master']['count'] = num_masterd
        confs['indexd']['count'] = num_indexd
        confs['client']['count'] = num_indexd

        # Santiy check
        self.check_volume_availability(confs['volume'])

        cluster = Cluster(self)
        cluster.start_spot(confs)

        # Reclaim the cluster
        cluster.stop()
