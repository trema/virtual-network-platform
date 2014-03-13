directory "/home/vagrant/tmp" do
  owner "vagrant"
  group "vagrant"
  mode 0755
  action :create
end

template "/home/vagrant/tmp/veth-interfaces" do
  source "veth-interfaces.erb"
  mode 0644
  owner "vagrant"
  group "vagrant"
  variables :veth_interfaces => node['veth_interfaces']
end

template "/home/vagrant/tmp/ovs-add-veth-ports.sh" do
  source "ovs-add-ports.erb"
  mode 0755
  owner "vagrant"
  group "vagrant"
  variables :ports => node['veth_interfaces']
end

cookbook_file "/etc/network/if-pre-up.d/veth" do
  source "veth"
  mode 0755
end

bash 'veth interfaces' do
  user "root"
  code <<-EOT
     cat /home/vagrant/tmp/veth-interfaces >> /etc/network/interfaces &&
     /etc/init.d/networking restart
  EOT
  not_if "fgrep '# The veth interfaces' /etc/network/interfaces"
end

bash 'ovs add ports' do
  user "root"
  code "sh /home/vagrant/tmp/ovs-add-veth-ports.sh"
end
