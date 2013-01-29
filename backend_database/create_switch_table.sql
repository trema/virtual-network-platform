use vnet;

create table switches (
       datapath_id      bigint unsigned not null,
       controller_host  varchar(255) not null,
       controller_pid   smallint unsigned not null,
       registered_at	timestamp,
       primary key (datapath_id),
       constraint controller_unique unique (datapath_id,controller_host,controller_pid),
       index host_pid_index (controller_host,controller_pid)
) engine = innodb;

create table switch_ports (
       datapath_id      bigint unsigned not null,
       port_no          smallint unsigned not null,
       name             varchar(255) not null,
       registered_at    timestamp,
       primary key (datapath_id,port_no),
       constraint name_unique unique (datapath_id,name),
       index name_index (name)
) engine = innodb;
