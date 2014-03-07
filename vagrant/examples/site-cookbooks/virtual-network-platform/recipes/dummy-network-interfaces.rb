directory "/home/vagrant/tmp" do
  owner "vagrant"
  group "vagrant"
  mode 0755
  action :create
end

template "/home/vagrant/tmp/dummy-network-interfaces" do
  source "dummy-network-interfaces.erb"
  mode 0644
  owner "vagrant"
  group "vagrant"
  variables :dummy_network_interfaces => node['dummy_network_interfaces']
end

template "/home/vagrant/tmp/ovs-add-dummy-ports.sh" do
  source "ovs-add-ports.erb"
  mode 0755
  owner "vagrant"
  group "vagrant"
  variables :ports => node['dummy_network_interfaces']
end

cookbook_file "/etc/network/if-pre-up.d/dummy" do
  source "dummy"
  mode 0755
end

hash 'dummy network interfaces' do
  user "root"
  code <<-EOT
     cat /home/vagrant/tmp/dummy-network-interfaces >> /etc/network/interfaces
     /etc/init.d/networking restart
  EOT
  not_if "fgrep '# The dummy network interfaces' /etc/network/interfaces"
end

bash 'ovs add ports' do
  user "root"
  code "sh /home/vagrant/tmp/ovs-add-dummy-ports.sh"
end
