#ifndef _ZMQ_PROCESS_STATUS_H
#define _ZMQ_PROCESS_STATUS_H

#include <pbs_config.h>

#ifdef ZMQ

int mom_read_json_status(size_t sz, char *data);
void update_my_json_status(char *status_strings);
char *create_json_statuses_buffer();

#endif /* ZMQ */

#endif /* _ZMQ_PROCESS_STATUS_H */
