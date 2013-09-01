#ifndef _ZMQ_PROCESS_MOM_COMMUNICATION_H
#define _ZMQ_PROCESS_MOM_COMMUNICATION_H

#include <pbs_config.h>

#ifdef ZMQ

int pbs_read_json_status(const size_t sz, const char *data);

#endif /* ZMQ */

#endif /* _ZMQ_PROCESS_MOM_COMMUNICATION_H */
