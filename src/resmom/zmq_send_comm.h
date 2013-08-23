#ifndef _ZMQ_SEND_COMM_H
#define _ZMQ_SEND_COMM_H

#include <pbs_config.h>

#ifdef ZMQ

int zmq_send_status(char *status_string);

#endif /* ZMQ */

#endif /* _ZMQ_SEND_COMM_H */
