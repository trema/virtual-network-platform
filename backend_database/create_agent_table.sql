use vnet;

create table agents (
       datapath_id      bigint unsigned not null,
       uri		text not null,
       primary key (datapath_id)
) engine = innodb;
