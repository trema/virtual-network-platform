log 'install prerequisites for tcpdump... wait a few minutes.'
%w{ gcc make autoconf automake libtool }.each do | package_name |
  package package_name
end

directory "/home/vagrant/tmp" do
  owner "vagrant"
  group "vagrant"
  mode 0755
  action :create
end

tcpdump_version = node['tcpdump_version'] || 'tcpdump-4.5.1'
tcpdump_tar_gz = "#{ tcpdump_version }.tar.gz"

remote_file "/home/vagrant/tmp/#{ tcpdump_tar_gz }" do
  source "http://www.tcpdump.org/release/#{ tcpdump_tar_gz }"
  owner 'vagrant'
  group 'vagrant'
  notifies :run, "bash[build-tcpdump]", :immediately
end

bash 'build-tcpdump' do
  cwd "/home/vagrant/tmp"
  user "vagrant"
  code <<-EOT
    tar xf #{ tcpdump_tar_gz } &&
    cd #{ tcpdump_version } &&
    CFLAGS=-DOPENFLOW_PORT=6653 ./configure --prefix /usr &&
    make
  EOT
  action :nothing
  notifies :run, "bash[install-tcpdump]", :immediately
end

bash 'install-tcpdump' do
  cwd "/home/vagrant/tmp/#{ tcpdump_version }"
  user "root"
  code "make install"
  action :nothing
end
