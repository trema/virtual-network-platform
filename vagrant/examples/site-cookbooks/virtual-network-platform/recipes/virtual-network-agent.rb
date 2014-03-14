log 'install gems for virtual network agent... wait a few minutes.'
bash "bundle install" do
  cwd "/home/vagrant/virtual-network-platform/virtual_network_agent"
  user "vagrant"
  code <<-'EOT'
    bundle config --local path vendor/bundle &&
    bundle install
  EOT
end

bash "setup init script" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/virtual_network_agent/init/virtual_network_agent /etc/init.d
  EOT
  not_if { ::File.exists?("/etc/init.d/virtual_network_agent") }
end

bash "setup configuration files" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/virtual_network_agent/config/virtual_network_agent /etc/default &&
    sed -i -e "s/^VIRTUAL_NETWORK_AGENT_DIR=.*$/VIRTUAL_NETWORK_AGENT_DIR=\"\/home\/vagrant\/virtual-network-platform\/virtual_network_agent\"/" /etc/default/virtual_network_agent
  EOT
  not_if { ::File.exists?("/etc/default/virtual_network_agent") }
end

if node['virtual_network_agent']['agent'] == 'reflector_agent'
  bash "edit configuration files" do
    cwd "/home/vagrant/virtual-network-platform/virtual_network_agent"
    user "vagrant"
    code <<-EOT
      sed -i -e "s/^agent: tunnel_endpoint_agent/#agent: tunnel_endpoint_agent/" \
             -e "s/^#agent: reflector_agent/agent: reflector_agent/" \
          configure.yml &&
      sed -i -e "s/^controller_uri:.*$/controller_uri: #{ node['virtual_network_agent']['controller_uri'].gsub('/','\/') }/" \
	  reflector_configure.yml
    EOT
  end
else
  bash "edit configuration files" do
    cwd "/home/vagrant/virtual-network-platform/virtual_network_agent"
    user "vagrant"
    code <<-EOT
      sed -i -e "s/^controller_uri:.*$/controller_uri: #{ node['virtual_network_agent']['controller_uri'].gsub('/','\/') }/" \
	     -e "s/^uri:.*$/uri: #{ node['virtual_network_agent']['uri'].gsub('/','\/') }/" \
	     -e "s/^tunnel_endpoint:.*$/tunnel_endpoint: #{ node['virtual_network_agent']['tunnel_endpoint'] }/" \
	  tunnel_endpoint_configure.yml
    EOT
  end

  if node['virtual_network_agent']['vxlan_adapter'] == 'linux_kernel'
    bash "update configuration files" do
      cwd "/home/vagrant/virtual-network-platform/virtual_network_agent"
      user "vagrant"
      code <<-EOT
	sed -i -e "s/adapter:.*$/adapter: #{ node['virtual_network_agent']['vxlan_adapter'] }/" \
	       -e "s/device: eth0/device: eth0.100/" \
	    tunnel_endpoint_configure.yml
      EOT
    end
  end
end

service "virtual_network_agent" do
  supports :status => true, :restart => true
  # update-rc.d virtual_network_agent defaults 30 10
  start_priority = [:start, 30]
  stop_priority = [:stop, 10]
  priority({
    2 => start_priority, 3 => start_priority, 4 => start_priority, 5 => start_priority,
    0 => stop_priority, 1 => stop_priority, 6 => stop_priority
  })
  action [:enable, :start]
end
