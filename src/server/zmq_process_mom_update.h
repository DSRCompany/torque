#ifndef _ZMQ_PROCESS_MOM_COMMUNICATION_H
#define _ZMQ_PROCESS_MOM_COMMUNICATION_H

#include <pbs_config.h>

#ifdef ZMQ

int pbs_read_json_status(size_t sz, char *data);

#endif /* ZMQ */

#endif /* _ZMQ_PROCESS_MOM_COMMUNICATION_H */
