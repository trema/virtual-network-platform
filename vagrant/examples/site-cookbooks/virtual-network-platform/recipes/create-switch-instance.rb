bash 'create switch instance' do
  user "root"
  code <<-EOT
    ovs-vsctl add-br br0
    ovs-vsctl set bridge br0 datapath_type=netdev
  EOT
end

bash 'set datapath identifier and openflow controller' do
  user "root"
  code <<-EOT
    ovs-vsctl set bridge br0 other-config:datapath-id=#{ node['openflow_switch']['datapath_id'] }
    ovs-vsctl set-controller br0 #{ node['openflow_switch']['controller'] } -- set controller br0 connection-mode=out-of-band
    ovs-vsctl set-fail-mode br0 secure
  EOT
end

bash 'restart open vswitch' do
  user "root"
  code "service openvswitch-switch restart"
end
