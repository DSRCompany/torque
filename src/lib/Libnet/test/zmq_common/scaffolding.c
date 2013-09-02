#include "pbs_config.h"
#include "license_pbs.h" /* See here for the software license */

#include <map>
#include <zmq.h>

int                 g_get_max_num_descriptors_ret;
int                 g_zmq_close_ret;
int                 g_zmq_ctx_destroy_ret;
void               *g_zmq_ctx_new_ret;
int                 g_zmq_getsockopt_ret;
int                 g_zmq_getsockopt_count;
bool                g_zmq_getsockopt_opt_rcvmore;
std::map<int, bool> g_zmq_getsockopt_opt_rcvmore_map;
char               *g_zmq_msg_data_ret;
int                 g_zmq_msg_data_count;
int                 g_zmq_msg_close_ret;
int                 g_zmq_msg_close_count;
int                 g_zmq_msg_init_ret;
int                 g_zmq_msg_init_count;
int                 g_zmq_msg_recv_ret;
std::map<int, int>  g_zmq_msg_recv_ret_map;
int                 g_zmq_msg_recv_errno;
int                 g_zmq_msg_recv_count;
int                 g_zmq_msg_recv_arg_flags;
int                 g_zmq_msg_size_ret;
int                 g_zmq_msg_size_count;
int                 g_zmq_setsockopt_ret;
void               *g_zmq_socket_ret;


void log_err(int errnum, const char *routine, const char *text)
  {
  // Do nothing, assume never fail
  }

/* return 0 or -1 */
int zmq_close(void *)
  {
  return g_zmq_close_ret;
  }

/* return 0 or -1 */
int zmq_ctx_destroy(void *context)
  {
  return g_zmq_ctx_destroy_ret;
  }

/* return pointer or NULL */
void *zmq_ctx_new(void)
  {
  return g_zmq_ctx_new_ret;
  }

/* return 0 or -1 */
int zmq_getsockopt(void *s, int option, void *optval, size_t *optvallen)
  {
  g_zmq_getsockopt_count++;
  if (option == ZMQ_RCVMORE)
    {
    if (g_zmq_getsockopt_opt_rcvmore_map.count(g_zmq_getsockopt_count))
      {
      *(int *)optval = (int)g_zmq_getsockopt_opt_rcvmore_map[g_zmq_getsockopt_count];
      }
    else
      {
      *(int *)optval = (int)g_zmq_getsockopt_opt_rcvmore;
      }
    }
  return g_zmq_getsockopt_ret;
  }

/* return 0 or -1 */
int zmq_msg_close(zmq_msg_t *msg)
  {
  g_zmq_msg_close_count++;
  return g_zmq_msg_close_ret;
  }

/* return pointer or NULL */
void *zmq_msg_data(zmq_msg_t *msg)
  {
  g_zmq_msg_data_count++;
  return g_zmq_msg_data_ret;
  }

/* return 0 or -1 */
int zmq_msg_init(zmq_msg_t *msg)
  {
  g_zmq_msg_init_count++;
  return g_zmq_msg_init_ret;
  }

/* return read count or -1, in case of -1 EAGAIN means no error just no more data */
int zmq_msg_recv(zmq_msg_t *msg, void *s, int flags)
  {
  g_zmq_msg_recv_count++;
  g_zmq_msg_recv_arg_flags = flags;
  errno = g_zmq_msg_recv_errno;
  if (g_zmq_msg_recv_ret_map.count(g_zmq_msg_recv_count))
    {
    return g_zmq_msg_recv_ret_map[g_zmq_msg_recv_count];
    }
  return g_zmq_msg_recv_ret;
  }

/* return message size, never fail */
size_t zmq_msg_size(zmq_msg_t *msg)
  {
  g_zmq_msg_size_count++;
  return g_zmq_msg_size_ret;
  }

/* return 0 or -1 */
int zmq_setsockopt(void *s, int option, const void *optval, size_t optvallen)
  {
  return g_zmq_setsockopt_ret;
  }

/* return pointer or NULL */
void *zmq_socket(void *context, int type)
  {
  return g_zmq_socket_ret;
  }

extern "C" {

/* return max descriptors number */
int get_max_num_descriptors(void)
  {
  return g_get_max_num_descriptors_ret;
  }

}

