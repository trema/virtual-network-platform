--
-- Copyright (C) 2012-2013 NEC Corporation
--

use vnet;

create table slices (
       id              int unsigned not null,
       description     text,
       mac_learning    tinyint unsigned not null,
       state           tinyint unsigned not null,
       updated_at      timestamp,
       primary key (id),
       index state_index (state)
) engine = innodb;

create table ports (
       id              int unsigned not null auto_increment,
       slice_id        int unsigned not null,
       datapath_id     bigint unsigned not null,
       port_no         smallint unsigned not null,
       port_name       varchar(255),
       vid             smallint unsigned not null,
       type            tinyint unsigned not null,
       description     text,
       mac_learning    tinyint unsigned not null,
       state           tinyint unsigned not null,
       updated_at      timestamp,
       primary key (id,slice_id),
       constraint port_unique unique (datapath_id,port_no,port_name,vid),
       index id_index (id),
       index slice_id_index (slice_id),
       index datapath_id_index (datapath_id),
       index type_index (type),
       index state_index (state)
) engine = innodb;

create table mac_addresses (
       slice_id        int unsigned not null,
       port_id         int unsigned not null,
       mac             bigint unsigned not null,
       type            tinyint unsigned not null,
       state           tinyint unsigned not null,
       updated_at      timestamp,
       primary key (slice_id,port_id,mac),
       index slice_id_index (slice_id),
       index port_id_index (port_id),
       index type_index (type),
       index state_index (state)
) engine = innodb;
