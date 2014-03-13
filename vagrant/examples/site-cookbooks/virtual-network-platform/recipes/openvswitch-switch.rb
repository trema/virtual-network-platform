file "/etc/default/openvswitch-switch" do
  owner "root"
  group "root"
  mode 0644
  content "OVS_MISSING_KMOD_OK=yes\n"
  action :create
end

log 'install prerequisites for openflow switch... wait a few minutes.'
package "openvswitch-switch"

bash 'using open vswitch without kernel support' do
  user "root"
  code <<-'EOT'
    sed -i -e "s/^\(insert_mod_if_required\)/\1() {\n    return 0 # without kernel support\n}\n\norg_\1/" /usr/share/openvswitch/scripts/ovs-ctl
  EOT
end

service "openvswitch-switch" do
  supports :status => true, :restart => true
  action :restart
end
