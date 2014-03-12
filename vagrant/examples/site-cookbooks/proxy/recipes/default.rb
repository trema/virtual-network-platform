if node['proxy'] and node['proxy']['http_proxy']
  http_proxy = node['proxy']['http_proxy']
  log "http proxy is [#{ http_proxy }]"
  file "/etc/apt/apt.conf.d/01proxy" do
    group "root"
    mode 0644
    content "Acquire::http::Proxy \"#{ http_proxy }\";\n"
    action :create
  end
  ENV['http_proxy'] = http_proxy
  ENV['https_proxy'] = http_proxy
else
  file "/etc/apt/apt.conf.d/01proxy" do
    action :delete
    only_if {::File.exists?("/etc/apt/apt.conf.d/01proxy")}
  end
end
