log 'install prerequisites for libpcap... wait a few minutes.'
%w{ gcc make flex bison autoconf automake libtool }.each do | package_name |
  package package_name
end

directory "/home/vagrant/tmp" do
  owner "vagrant"
  group "vagrant"
  mode 0755
  action :create
end

libpcap_version = node['libpcap_version'] || 'libpcap-1.5.3'
libpcap_tar_gz = "#{ libpcap_version }.tar.gz"

remote_file "/home/vagrant/tmp/#{ libpcap_tar_gz }" do
  source "http://www.tcpdump.org/release/#{ libpcap_tar_gz }"
  owner 'vagrant'
  group 'vagrant'
end

log 'build libpcap... wait a few minutes.'
bash 'build libpcap' do
  cwd "/home/vagrant/tmp"
  user "vagrant"
  code <<-EOT
    tar xf #{ libpcap_tar_gz }
    cd #{ libpcap_version }
    ./configure --prefix /usr
    make
  EOT
end

bash 'install libpcap' do
  cwd "/home/vagrant/tmp/#{ libpcap_version }"
  user "root"
  code "make install"
end

bash 'make clean' do
  cwd "/home/vagrant/tmp/#{ libpcap_version }"
  user "vagrant"
  code "make clean"
end
