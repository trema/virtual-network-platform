# How to use?

## Prerequisite

* Ubuntu 12.04.1 LTS Desktop (amd64)
* vagrant, NOT the gem version!
* lxc
* vagrant-lxc
* vlan

## Avoiding 'sudo' passwords

[[https://github.com/fgrehm/vagrant-lxc/wiki/Avoiding-'sudo'-passwords]]

## Multicast over linux bridge

multicast on 802.1q does not work???

## Installation

        $ cd .../virtual-network-platform/vagrant/examples/basic

        $ sudo visudo
        > Defaults !tty_tickets

        $ wget https://dl.bintray.com/mitchellh/vagrant/vagrant_1.4.3_x86_64.deb -O /tmp/vagrant_1.4.3_x86_64.deb
        $ sudo dpkg -i /tmp/vagrant_1.4.3_x86_64.deb
        $ sudo apt-get install lxc
        $ sudo vagrant plugin install vagrant-lxc

        $ bundle config --local path vendor/bundle
        $ bundle install
        $ bundle exec librarian-chef install

        $ sudo modprobe 8021q
        $ sudo rmmod vxlan
        $ sudo sh -c "echo 0 > /sys/class/net/lxcbr0/bridge/multicast_snooping"

        $ vagrant up --provider=lxc

## Use

### Run ssh in new terminal

        $ vagrant ssh hosta
        ...

### Run ssh in new terminal

        $ vagrant ssh hostb
        ...

### Run vagrant ssh in new terminal

        $ vagrant ssh hostc
        ...

## Destroy

        $ vagrant destroy --force
        ...
        $
