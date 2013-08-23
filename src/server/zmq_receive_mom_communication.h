#ifndef _ZMQ_RECEIVE_MOM_COMMUNICATION_H
#define _ZMQ_RECEIVE_MOM_COMMUNICATION_H

#include <pbs_config.h>

#ifdef ZMQ

void *start_process_pbs_status_port(void *zsock);

#endif /* ZMQ */

#endif /* _ZMQ_RECEIVE_MOM_COMMUNICATION_H */
