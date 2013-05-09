# -*- mode: ruby -*-
# vi: set ft=ruby :
#
# Vagrant box used for development.

Vagrant.configure("2") do |config|
  # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.

  config.vm.box = "raring"

  config.vm.provider :kvm do |kvm|
	config.vm.network :private_network, ip: "192.168.192.122"
  end

  config.vm.provider :lxc do |lxc|
	config.vm.box_url = "http://dl.dropbox.com/u/13510779/lxc-raring-amd64-2013-05-08.box"
  end

  config.vm.provider :virtualbox do |vb|
	vb.customize ["modifyvm", :id, "--memory", "2048"]
  end

#  config.vm.provision :puppet do |puppet|
#	puppet.manifests_path = "../puppet/"
#	puppet.module_path = "../puppet/"
#	puppet.manifest_file  = "default.pp"
#  end
end
