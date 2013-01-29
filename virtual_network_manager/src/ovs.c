/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#include <assert.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <strings.h>
#include "ovs.h"
#include "trema.h"


static ovs_event_handlers event_handlers = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


static uint16_t
get_actions_length( const openflow_actions *actions ) {
  assert( actions != NULL );

  debug( "Calculating the total length of actions." );

  int length = 0;
  list_element *action = actions->list;
  while ( action != NULL ) {
    struct ofp_action_header *ah = ( struct ofp_action_header * ) action->data;
    length += ah->len;
    action = action->next;
  }

  debug( "Total length of actions = %d.", length );

  assert( length <= UINT16_MAX );

  return ( uint16_t ) length;
}


buffer *
create_ovs_set_flow_format( const uint32_t transaction_id, const uint32_t format ) {
  size_t length = sizeof( ovs_set_flow_format );
  buffer *message = alloc_buffer_with_length( length );
  ovs_set_flow_format *sff = append_back_buffer( message, length );
  sff->header.version = OFP_VERSION;
  sff->header.type = OFPT_VENDOR;
  sff->header.length = htons( sizeof( ovs_set_flow_format ) );
  sff->header.xid = htonl( transaction_id );
  sff->vendor = htonl( OVS_VENDOR_ID );
  sff->subtype = htonl( OVST_SET_FLOW_FORMAT );
  sff->format = htonl( format );

  return message;
}


buffer *
create_ovs_flow_mod( const uint32_t transaction_id, const uint64_t cookie, const uint16_t command,
                    const uint8_t table_id, const uint16_t idle_timeout, const uint16_t hard_timeout,
                    const uint16_t priority, const uint32_t buffer_id,
                    const uint16_t out_port, const uint16_t flags,
                    const ovs_matches *matches, const openflow_actions *actions ) {
  uint16_t matches_length = 0;
  uint16_t padding_length = 0;
  if ( matches != NULL ) {
    matches_length = get_ovs_matches_length( matches );
    padding_length = ( uint16_t ) ( ( matches_length + 7 ) / 8 * 8 - matches_length );
  }
  uint16_t actions_length = 0;
  if ( actions != NULL ) {
    actions_length = get_actions_length( actions );
  }

  uint16_t length = ( uint16_t ) ( sizeof( ovs_flow_mod ) +
                                   matches_length + padding_length + actions_length );

  buffer *buf = alloc_buffer_with_length( length );
  ovs_flow_mod *flow_mod = append_back_buffer( buf, sizeof( ovs_flow_mod ) );

  flow_mod->ovsh.header.version = OFP_VERSION;
  flow_mod->ovsh.header.type = OFPT_VENDOR;
  flow_mod->ovsh.header.length = htons( length );
  flow_mod->ovsh.header.xid = htonl( transaction_id );
  flow_mod->ovsh.vendor = ntohl( OVS_VENDOR_ID );
  flow_mod->ovsh.subtype = ntohl( OVST_FLOW_MOD );

  flow_mod->cookie = htonll( cookie );
  flow_mod->command = htons( ( uint16_t ) ( ( table_id << 8 ) | ( command & 0xff ) ) );
  flow_mod->idle_timeout = htons( idle_timeout );
  flow_mod->hard_timeout = htons( hard_timeout );
  flow_mod->priority = htons( priority );
  flow_mod->buffer_id = htonl( buffer_id );
  flow_mod->out_port = htons( out_port );
  flow_mod->flags = htons( flags );
  flow_mod->match_len = htons( matches_length );

  if ( matches_length > 0 ) {
    void *p = append_back_buffer( buf, ( size_t ) ( matches_length + padding_length ) );
    list_element *element = matches->list;
    while( element != NULL ) {
      ovs_match_header *header = element->data;
      size_t entry_length = sizeof( ovs_match_header ) + get_ovs_match_length( *header );
      hton_ovs_match( ( ovs_match_header * ) p, header );
      p = ( void * ) ( ( char * ) p + entry_length );
      element = element->next;
    }
  }

  if ( actions_length > 0 ) {
    void *p = append_back_buffer( buf, actions_length );
    list_element *element = actions->list;
    while ( element != NULL ) {
      struct ofp_action_header *ah = element->data;
      uint16_t action_length = ah->len;
      if ( ah->type == OFPAT_VENDOR ) {
        struct ofp_action_vendor_header *vh = ( struct ofp_action_vendor_header * ) ah;
        if ( vh->vendor == OVS_VENDOR_ID ) {
          hton_ovs_action( ( ovs_action_header * ) p, ( const ovs_action_header * ) ah );
        }
        else {
          hton_action( ( struct ofp_action_header * ) p, ah );
        }
      }
      else {
        hton_action( ( struct ofp_action_header * ) p, ah );
      }
      p = ( void * ) ( ( char * ) p + action_length );
      element = element->next;
    }
  }

  return buf;
}


buffer *
create_ovs_flow_mod_table_id( const uint32_t transaction_id, const uint8_t set ) {
  uint16_t length = ( uint16_t ) sizeof( ovs_flow_mod_table_id );
  buffer *buf = alloc_buffer_with_length( length );
  ovs_flow_mod_table_id *flow_mod_table_id = append_back_buffer( buf, length );

  flow_mod_table_id->header.version = OFP_VERSION;
  flow_mod_table_id->header.type = OFPT_VENDOR;
  flow_mod_table_id->header.length = htons( length );
  flow_mod_table_id->header.xid = htonl( transaction_id );
  flow_mod_table_id->vendor = ntohl( OVS_VENDOR_ID );
  flow_mod_table_id->subtype = ntohl( OVST_FLOW_MOD_TABLE_ID );
  flow_mod_table_id->set = set;

  return buf;
}


static void
handle_ovs_flow_removed( uint64_t datapath_id, uint32_t transaction_id, const buffer *data ) {
  assert( data != NULL );

  ovs_flow_removed *flow_removed = ( ovs_flow_removed * ) ( ( char * ) data->data - offsetof( ovs_header, subtype ) );
  uint64_t cookie = ntohll( flow_removed->cookie );
  uint16_t priority = ntohs( flow_removed->priority );
  uint8_t reason = flow_removed->reason;
  uint32_t duration_sec = ntohl( flow_removed->duration_sec );
  uint32_t duration_nsec = ntohl( flow_removed->duration_nsec );
  uint16_t idle_timeout = ntohs( flow_removed->idle_timeout );
  size_t match_length = ntohs( flow_removed->match_len );
  uint64_t packet_count = ntohll( flow_removed->packet_count );
  uint64_t byte_count = ntohll( flow_removed->byte_count );

  ovs_matches *matches = create_ovs_matches();
  size_t offset = sizeof( ovs_flow_removed );
  while ( match_length >= sizeof( ovs_match_header ) ) {
    ovs_match_header *src = ( ovs_match_header * ) ( ( char * ) flow_removed + offset );
    size_t length = get_ovs_match_length( ntohl( *src ) ) + sizeof( ovs_match_header );
    ovs_match_header *dst = xmalloc( length );
    ntoh_ovs_match( dst, src );
    append_to_tail( &matches->list, dst );
    matches->n_matches++;
    offset += length;
    match_length -= length;
  }

  char matches_str[ 256 ];
  ovs_matches_to_string( matches, matches_str, sizeof( matches_str ) );

  debug( "An Open vSwitch extended flow removed received ( datapath_id = %#" PRIx64 ", "
         "transaction_id = %#x, cookie = %#" PRIx64 ", priority = %u, reason = %#x, "
         "duration = %u.%09u, idle_timeout = %u, packet_count = %" PRIu64
         ", byte_count = %" PRIu64 ", matches = [%s], user_data = %p ).",
         datapath_id, transaction_id, cookie, priority, reason, duration_sec, duration_nsec,
         idle_timeout, packet_count, byte_count, matches_str,
         event_handlers.ovs_flow_removed_user_data );

  if ( event_handlers.ovs_flow_removed_callback != NULL ) {
    event_handlers.ovs_flow_removed_callback( datapath_id, transaction_id,
                                             cookie, priority, reason,
                                             duration_sec, duration_nsec,
                                             idle_timeout, packet_count, byte_count,
                                             matches,
                                             event_handlers.ovs_flow_removed_user_data );
  }

  delete_ovs_matches( matches );
}


static void
handle_ovs_vendor( uint64_t datapath_id, uint32_t transaction_id, const buffer *data ) {
  assert( data != NULL );
  assert( data->length >= sizeof( uint32_t ) );

  uint32_t subtype = ntohl( *( ( uint32_t * ) data->data ) );
  switch ( subtype ) {
    case OVST_FLOW_REMOVED:
    {
      handle_ovs_flow_removed( datapath_id, transaction_id, data );
    }
    break;

    default:
    {
      warn( "Unsupported subtype ( %#x ).", subtype );
    }
    break;
  }
}


static void
handle_vendor( uint64_t datapath_id, uint32_t transaction_id, uint32_t vendor,
               const buffer *data, void *user_data ) {
  UNUSED( user_data );

  if ( vendor == OVS_VENDOR_ID ) {
    handle_ovs_vendor( datapath_id, transaction_id, data );
    return;
  }

  if ( event_handlers.other_vendor_callback != NULL ) {
    event_handlers.other_vendor_callback( datapath_id, transaction_id, vendor, data,
                                          event_handlers.other_vendor_user_data );
  }
}


static void
handle_ovs_vendor_error( uint64_t datapath_id, uint32_t transaction_id, const buffer *data ) {
  assert( data != NULL );
  assert( data->length > 0 );

  buffer *ovs_data = duplicate_buffer( data );
  remove_front_buffer( ovs_data, sizeof( ovs_vendor_error ) );
  ovs_vendor_error *err = data->data;

  debug( "An Open vSwitch vendor error message received ( datapath_id = %#" PRIx64 ", transaction_id = %#x, "
         "type = %#x, code = %#x, ovs_data = %p ).",
         datapath_id, transaction_id, ntohs( err->type ), ntohs( err->code ), ovs_data );

  if ( event_handlers.ovs_error_callback != NULL ) {
    event_handlers.ovs_error_callback( datapath_id, transaction_id, ntohs( err->type ), ntohs( err->code ),
                                      ovs_data, event_handlers.ovs_error_user_data );
  }

  free_buffer( ovs_data );
}


static void
handle_error( uint64_t datapath_id, uint32_t transaction_id, uint16_t type, uint16_t code,
              const buffer *data, void *user_data ) {
  UNUSED( user_data );

  if ( type == OVSET_VENDOR && code == OVSVC_VENDOR_ERROR ) {
    handle_ovs_vendor_error( datapath_id, transaction_id, data );
    return;
  }

  if ( event_handlers.other_error_callback != NULL ) {
    event_handlers.other_error_callback( datapath_id, transaction_id, type, code, data,
                                         event_handlers.other_error_user_data );
  }
}


bool
set_ovs_flow_removed_handler( ovs_flow_removed_handler callback, void *user_data ) {
  if ( callback == NULL ) {
    error( "Callback function ( ovs_error_handler ) must not be NULL." );
    return false;
  }

  set_vendor_handler( handle_vendor, NULL );

  debug( "Setting an Open vSwitch flow removed handler ( callback = %p, user_data = %p ).",
         callback, user_data );

  event_handlers.ovs_flow_removed_callback = callback;
  event_handlers.ovs_flow_removed_user_data = user_data;

  return true;
}


bool
set_other_vendor_handler( vendor_handler callback, void *user_data ) {
  if ( callback == NULL ) {
    error( "Callback function ( vendor_handler ) must not be NULL." );
    return false;
  }

  set_vendor_handler( handle_vendor, NULL );

  debug( "Setting an other vendor handler ( callback = %p, user_data = %p ).",
         callback, user_data );

  event_handlers.other_vendor_callback = callback;
  event_handlers.other_vendor_user_data = user_data;

  return true;
}


bool
set_ovs_error_handler( ovs_error_handler callback, void *user_data ) {
  if ( callback == NULL ) {
    error( "Callback function ( ovs_error_handler ) must not be NULL." );
    return false;
  }

  debug( "Setting an Open vSwitch error handler ( callback = %p, user_data = %p ).",
         callback, user_data );

  set_error_handler( handle_error, NULL );

  event_handlers.ovs_error_callback = callback;
  event_handlers.ovs_error_user_data = user_data;

  return true;
}


bool
set_other_error_handler( error_handler callback, void *user_data ) {
  if ( callback == NULL ) {
    error( "Callback function ( error_handler ) must not be NULL." );
    return false;
  }

  debug( "Setting an other error handler ( callback = %p, user_data = %p ).",
         callback, user_data );

  set_error_handler( handle_error, NULL );

  event_handlers.other_error_callback = callback;
  event_handlers.other_error_user_data = user_data;

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
