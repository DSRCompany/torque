#ifndef _ZMQ_COMMON_H

#include <pbs_config.h>

#ifdef ZMQ

#include <zmq.h>

#include "net_connect.h"

void start_socket_thread(int id, void*(*func)(void*));
int close_zmq_socket(void *socket);
int init_zmq();
void deinit_zmq();
int init_zmq_connection(enum zmq_connection_e id, int  socket_type);
int close_zmq_connection(enum zmq_connection_e id);
int add_zconnection(enum zmq_connection_e id, void *socket, void *(*func)(void *), bool should_poll,
    bool connected);

extern void  *g_zmq_context;
extern struct zconnection_s g_svr_zconn[];
extern zmq_pollitem_t *gs_zmq_poll_list;

#endif /* ZMQ */

#endif /* _ZMQ_COMMON_H */
