iproute_deb = "iproute_20121211-2ubuntu1_amd64.deb"

remote_file "/home/vagrant/tmp/#{ iproute_deb }" do
  source "http://mirrors.kernel.org/ubuntu/pool/main/i/iproute/#{ iproute_deb }"
  owner 'vagrant'
  group 'vagrant'
end

bash 'build openvswitch' do
  cwd "/home/vagrant/tmp"
  user "root"
  code "dpkg -i #{ iproute_deb }"
end
