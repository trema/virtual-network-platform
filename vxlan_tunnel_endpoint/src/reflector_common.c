/*
 * Copryright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#include "reflector_common.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
queue *received_packets = NULL;
queue *free_packet_buffers = NULL;
volatile bool running = true;
pthread_t receiver_thread = 0;
pthread_t distributor_thread = 0;


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
