use vnet;

create table overlay_networks (
       slice_id               int unsigned not null,
       reflector_group_id     smallint unsigned not null,
       primary key (slice_id)
) engine = innodb;

create table reflectors (
       id                     smallint unsigned not null,
       group_id		      smallint unsigned not null,
       broadcast_address      varchar(128) not null,
       broadcast_port         smallint unsigned not null,
       uri		      text not null,
       primary key (id)
) engine = innodb;

create table tunnel_endpoints (
       datapath_id	      bigint unsigned not null,
       local_address          varchar(128) not null,
       local_port             smallint unsigned not null,
       primary key (datapath_id)
) engine = innodb;

insert into reflectors (id,group_id,broadcast_address,broadcast_port,uri) values (1,1,'239.0.0.1',60000,'');