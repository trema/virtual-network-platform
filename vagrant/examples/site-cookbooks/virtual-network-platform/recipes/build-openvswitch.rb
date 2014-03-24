log 'install prerequisites for openflow_switch... wait a few minutes.'
%w{ gcc make autoconf automake libtool pkg-config uuid-runtime }.each do | package_name |
  package package_name
end

directory "/home/vagrant/tmp" do
  owner "vagrant"
  group "vagrant"
  mode 0755
  action :create
end

openvswitch_version = node['openvswitch_version'] || 'openvswitch-2.1.0'
openvswitch_tar_gz = "#{ openvswitch_version }.tar.gz"

remote_file "/home/vagrant/tmp/#{ openvswitch_tar_gz }" do
  source "http://openvswitch.org/releases/#{ openvswitch_tar_gz }"
  owner 'vagrant'
  group 'vagrant'
end

log 'build openvswitch... wait a few minutes.'
bash 'build openvswitch' do
  cwd "/home/vagrant/tmp"
  user "vagrant"
  code <<-EOT
    tar xf #{ openvswitch_tar_gz } &&
    cd #{ openvswitch_version } &&
    ./boot.sh &&
    ./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc &&
    make
  EOT
end

file "/etc/default/openvswitch-switch" do
  owner "root"
  group "root"
  mode 0644
  content "OVS_MISSING_KMOD_OK=yes\nOVS_FORCE_RELOAD_KMOD=no\n"
  action :create
end

bash 'install openvswitch' do
  cwd "/home/vagrant/tmp/#{ openvswitch_version }"
  user "root"
  code <<-'EOT'
    make install &&
    sed -i -e "s/^\(insert_mod_if_required\)/\1() {\n    return 0 # without kernel support\n}\n\norg_\1/" /usr/share/openvswitch/scripts/ovs-ctl &&
    cp debian/openvswitch-switch.init /etc/init.d/openvswitch-switch
  EOT
end

bash 'make clean' do
  cwd "/home/vagrant/tmp/#{ openvswitch_version }"
  user "vagrant"
  code "make clean"
end

service "openvswitch-switch" do
  supports :status => true, :restart => true
  action [:enable, :start]
end
