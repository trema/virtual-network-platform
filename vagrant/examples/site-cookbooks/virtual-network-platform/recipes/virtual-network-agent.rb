log 'install prerequisites for virtual network agent... wait a few minutes.'
%w{ ruby ruby-json ruby-sinatra ruby-rest-client ruby-systemu }.each do | package_name |
  package package_name
end

bash "setup init script" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/virtual_network_agent/init/virtual_network_agent /etc/init.d
    update-rc.d virtual_network_agent defaults 30 10
  EOT
  not_if { ::File.exists?("/etc/init.d/virtual_network_agent") }
end

bash "setup configuration files" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/virtual_network_agent/config/virtual_network_agent /etc/default
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
          configure.yml
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

bash 'start virtual network agent' do
  user "root"
  code "service virtual_network_agent start"
end
