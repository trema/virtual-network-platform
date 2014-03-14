# What's this?

This is the [Virtual Network Platform 1.0](http://trema.github.io/virtual-network-platform/)
which allows you to construct and manage VXLAN-based virtual networks
with a single centralized management point. The centralized management
point provides a REST-based interface for managing the virtual networks.

Don't abbreviate Virtual Network Platform as VNP.

# How does it work?

VXLAN-based virtual networks are managed with the following system
architecture and software packages.

## System architecture

![System architecture](doc/architecture.png)

## Software packages

### Vritual Network Controller

Virtual Network Controller consists of the following tree
subcomponents.

#### Virtual Network Manager

Virtual Network Manager is an application responsible for managing
virtual networks. It retrieves configuration from the backend database
(see below for details) and make necessary changes in OpenFlow
switches and VXLAN tunnel endpoints. It acts as an OpenFlow
controller. It is developed on top of Trema which is an OpenFlow
framework for developing OpenFlow controllers and switches.

#### Backend Database

Backend Database stores virtual network configuration and operational
states of OpenFlow switches and Virtual Network Manager. It is a
database created on MySQL.

#### Configuration Frontend

Configuration Frontend provides a REST interface for managing virtual
networks. It works as a web server and allows you to create/delete
virtual networks, attach/detach switch ports to/from virtual networks,
associate/detach MAC addresses with/from switch ports via the REST
interface.

### Virtual Network Agent

Virtual Network Agent receives requests from Virtual Network Manager
to configure VXLAN tunnel endpoint and OpenFlow switch. It runs on the
same host as VXLAN tunnel endpoint and OpenFlow switch are running.

### VXLAN Tunnel End-Point

VXLAN Tunnel End-Point is a VXLAN tunnel endpoint implementation. It
can be a user space implementation included in this software suite or
standard Linux kernel implementation available on Linux 3.7.

### OpenFlow Switch

OpenFlow Switch is unmodified Open vSwitch (version 1.4.X). It is not
included in this software suite. For detailed information on Open
vSwitch, please visit [http://openvswitch.org/](http://openvswitch.org/).

# How to use?

## Prerequisite

At least, one host that runs Virtual Network Controller and two hosts
for Virtual Network Agent, VXLAN Tunnel End-Point, and OpenFlow switch
are required. The following operating system is only supported.

* Ubuntu 12.04.1 LTS Desktop (amd64)

Operating systems other than above are totally not tested and may not
work expectedly.

## Getting the source code

You can retrieve a copy of the source code as follows:

    $ sudo apt-get install git
    $ git clone --recurse-submodules \
    git://github.com/trema/virtual-network-platform.git
    $ cd virtual-network-platform

If you prefer to use HTTP, follow the instructions below:

    $ git clone https://github.com/trema/virtual-network-platform.git
    $ cd virtual-network-platform
    $ git config submodule.trema.url https://github.com/trema/trema.git
    $ git submodule init
    $ git submodule update

## Setup

We assume that the software suite is installed in the following
environment.

![Setup](doc/setup.png)

### Virtual Network Controller

#### Virtual Network Manager

Since Virtual Network Manager is developed on top of Trema, you need to
build Trema before building Virtual Network Manager.

1. Build Trema

        $ sudo apt-get install gcc make git ruby1.9.3 libpcap-dev libsqlite3-dev \
        libglib2.0-dev
        $ sudo update-alternatives --set ruby /usr/bin/ruby1.9.1
        $ sudo update-alternatives --set gem /usr/bin/gem1.9.1
        $ sudo gem install rubygems-update
        $ sudo update_rubygems
        $ sudo gem install bundler
        $ cd trema
        $ bundle config --local path vendor/bundle
        $ bundle install
        $ ./build.rb
        $ mkdir -p tmp/log tmp/sock tmp/pid
        $ cd ..

2. Build Virtual Network Manager

        $ sudo apt-get install libcurl4-gnutls-dev libjson0-dev \
        libmysqlclient-dev
        $ cd virtual_network_manager/src
        $ make
        $ cd ../..

3. Setup init script

        $ sudo cp virtual_network_manager/init/virtual_network_manager \
        /etc/init.d
        $ sudo update-rc.d virtual_network_manager defaults
        $ sudo cp virtual_network_manager/init/trema /etc/init.d
        $ sudo update-rc.d trema defaults

4. Copy configuration files

        $ sudo cp virtual_network_manager/config/virtual_network_manager \
        /etc/default
        $ sudo chown root.root /etc/default/virtual_network_manager
        $ sudo chmod 600 /etc/default/virtual_network_manager
        $ sudo cp virtual_network_manager/config/trema /etc/default

5. Set the directory that the Virtual Network Manager executable exists

    Specify the directory that you have installed Virtual Network Manager
    executable in /etc/default/virtual_network_manager like follows:

        VIRTUAL_NETWORK_MANAGER_DIR="/somewhere/virtual_network_manager/src"

6. Set the directory that Trema exists

    Specify the directory that you have installed Trema executable in
    /etc/default/trema like follows:

        TREMA_HOME="/somewhere/trema"

#### Backend Database

1. Install MySQL server and client

        $ sudo apt-get install mysql-server mysql-client

    During the installation process, you may be asked to set password for
    the MySQL "root" user. We assume here that the password is set to
    "root123".

2. Add privileges to "root"

        $ mysql -u root --password=root123
        mysql> grant all privileges on *.* to root@localhost identified
        by 'root123' with grant option;
        mysql> flush privileges;
        mysql> quit

3. Create database and tables

        $ cd backend_database
        $ ./create_database.sh
        $ cd ..

#### Configuration Frontend

1. Install Sinatra and ActiveRecord

        $ sudo apt-get install ruby1.9.3
        $ sudo update-alternatives --set ruby /usr/bin/ruby1.9.1
        $ sudo update-alternatives --set gem /usr/bin/gem1.9.1
        $ sudo gem install rubygems-update
        $ sudo update_rubygems
        $ sudo gem install bundler
        $ cd configuration_frontend
        $ bundle config --local path vendor/bundle
        $ bundle install
        $ cd ..

2. Setup init script

        $ sudo cp configuration_frontend/init/configuration_frontend \
        /etc/init.d
        $ sudo update-rc.d configuration_frontend defaults

3. Copy configuration file

        $ sudo cp configuration_frontend/config/configuration_frontend \
        /etc/default

4. Set the directory that the Configuration Frontend executable exists

    Specify the directory that you have installed Configuration Frontend
    in /etc/default/configuration_frontend like follows:

        CONFIGURATION_FRONTEND_DIR="/somewhere/configuration_frontend"

#### Start all required services

    $ sudo service trema start
    $ sudo service virtual_network_manager start
    $ sudo service configuration_frontend start

### OpenFlow Switch (Open vSwitch)

1. Install Open vSwitch

        DKMS version:
        $ sudo apt-get install openvswitch-switch openvswitch-datapath-dkms

        module-assistant version:
        $ sudo apt-get install openvswitch-switch openvswitch-datapath-source
        $ sudo module-assistant auto-install openvswitch-datapath

3. Create switch instance and add switch ports

        $ sudo ovs-vsctl add-br br0
        $ sudo ovs-vsctl add-port br0 eth1
        $ sudo ovs-vsctl add-port br0 eth2

3. Set datapath identifier and OpenFlow controller

        $ sudo ovs-vsctl set Bridge br0 \
        other-config:datapath-id=[datapath id in hex]
        $ sudo ovs-vsctl set-controller br0 tcp:192.168.1.254:6653 \
        -- set controller br0 connection-mode=out-of-band
        $ sudo ovs-vsctl set-fail-mode br0 secure

    Datapath id must be a 64-bit unique identifier for specifying the
    switch instance. You need to assign a unique identifier for each
    switch instance. Note hat datapath id may be specified with 16 digits
    hexadecimal without "0x" prefix (e.g. 0000000000000001).

4. Restart Open vSwitch

        $ sudo service openvswitch-switch restart

### VXLAN Tunnel End-Point

1. Build VXLAN Tunnel End-Point

        $ sudo apt-get install gcc make
        $ cd vxlan_tunnel_endpoint/src
        $ make
        $ cd ../..

2. Setup init script

        $ sudo cp vxlan_tunnel_endpoint/init/vxland /etc/init.d
        $ sudo update-rc.d vxland defaults

3. Setup configuration file

        $ sudo cp vxlan_tunnel_endpoint/config/vxland /etc/default

    Edit the configuration file (/etc/default/vxland). Set the directory
    that the executable (vxland) exists.

        VXLAND_DIR="/somewhere/vxlan_tunnel_endpoint/src"

4. Start VXLAN Tunnel End-Point

        $ sudo service vxland start

### Virtual Network Agent

1. Install Sinatra

        $ sudo apt-get install ruby1.9.3
        $ sudo update-alternatives --set ruby /usr/bin/ruby1.9.1
        $ sudo update-alternatives --set gem /usr/bin/gem1.9.1
        $ sudo gem install rubygems-update
        $ sudo update_rubygems
        $ sudo gem install bundler
        $ cd virtual_network_agent
        $ bundle config --local path vendor/bundle
        $ bundle install
        $ cd ..

2. Setup init script

        $ sudo cp virtual_network_agent/init/virtual_network_agent \
        /etc/init.d
        $ sudo update-rc.d virtual_network_agent defaults 30 10

3. Setup configuration file

        $ sudo cp virtual_network_agent/config/virtual_network_agent \
        /etc/default

4. Edit configuration files

    Specify the directory that you have installed Virtual Network Agent
    in /etc/default/virtual_network_agent like follows:

        VIRTUAL_NETWORK_AGENT_DIR="/somewhere/virtual_network_agent"

    Set the Configuration Frontend URL, Virtual Network Agent URL and
    Tunnel endpoint address in virtual_network_agent/tunnel_endpoint_configure.yml.

        controller_uri: http://192.168.1.254:8081/
        uri: http://192.168.1.16:8082/
        tunnel_endpoint: 192.168.1.16:4789

5. Start Virtual Network Agent

        $ sudo service virtual_network_agent start

## How to manage virtual networks?

Here are simple examples that show how to manage virtual networks.
Please see files under [doc/api](doc/api/api.html) for extended examples.

### Create a virtual network

    $ curl -v \
    -H "Accept: application/json" \
    -H "Content-type: application/json" \
    -X POST \
    -d '{ "id": 128, "description": "Virtual network #128" }' \
    http://192.168.1.254:8081/networks

### Associate a switch port with a virtual network

    $ curl -v \
    -H "Accept: application/json" \
    -H "Content-type: application/json" \
    -X POST \
    -d '{ "id": 1, "datapath_id": "1", "name": "eth1",
    "vid": 65535, "description": "eth1 on switch #1" }' \
    http://192.168.1.254:8081/networks/128/ports

### Associate a MAC address with a switch port

    $ curl -v \
    -H "Accept: application/json" \
    -H "Content-type: application/json" \
    -X POST \
    -d '{ "address" : "00:00:00:00:00:01" }' \
    http://192.168.1.254:8081/networks/128/ports/1/mac_addresses

# Copyright

Copyright (C) 2012-2013 NEC Corporation

# License & Terms

All software packages distributed here are licensed under the GNU
General Public License version 2.0:

[http://www.gnu.org/licenses/gpl-2.0.html](http://www.gnu.org/licenses/gpl-2.0.html)
