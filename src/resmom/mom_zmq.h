#ifndef _MOM_ZMQ_H
#define _MOM_ZMQ_H

#include <pbs_config.h>

#ifdef ZMQ

#include "net_connect.h"

int zmq_connect_sockaddr(enum zmq_connection_e id, struct sockaddr_in *sock_addr, int port);
int zmq_setopt_hwm(zmq_connection_e id, int value);
int update_status_connection();

#endif /* ZMQ */

#endif /* _MOM_ZMQ_H */
