include_recipe 'virtual-network-platform::tuntap-device'
if ['build_openvswitch']
  include_recipe 'virtual-network-platform::build_openvswitch'
else
  include_recipe 'virtual-network-platform::openvswitch-switch'
end
include_recipe 'virtual-network-platform::create-switch-instance'
include_recipe 'virtual-network-platform::getting-the-source-code'
include_recipe 'virtual-network-platform::vxlan-tunnel-end-point'
include_recipe 'virtual-network-platform::virtual-network-agent'
