#include "pbs_config.h"
#include "license_pbs.h" /* See here for the software license */
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <zmq.h>

#include "net_connect.h"
#include "log.h"
#include "server_limits.h"

const char *msg_daemonname = "unset";

void *g_zmq_socket_ret = 0;
int g_zmq_bind_ret = 0;
int g_zmq_close_ret = 0;
int g_add_zconnection_ret = 0;
int g_zmq_poll_ret = 0;
int g_zmq_poll_errno = 0;
int g_get_max_num_descriptors_ret = 0;
int g_fstat_bad_descriptor = 0;
int g_pthread_mutex_lock_cnt = 0;
int g_pthread_mutex_unlock_cnt = 0;

zmq_pollitem_t *g_zmq_poll_list = NULL;
struct zconnection_s g_svr_zconn[ZMQ_CONNECTION_COUNT] = {};
void *g_zmq_context = NULL;
char log_buffer[LOG_BUF_SIZE];

fd_set *GlobalSocketReadSet;
u_long *GlobalSocketAddrSet;
u_long *GlobalSocketPortSet;

void disiui_() {}

char *get_cached_nameinfo(
    
  struct sockaddr_in  *sai)

  {
  return(NULL);
  }

void log_event(int eventtype, int objclass, const char *objname, const char *text)
  {
  exit(1);
  }

void initialize_connections_table()
  {
  exit(1);
  }

char *PAddrToString(pbs_net_t *Addr)
  {
  exit(1);
  }

int get_fdset_size(void)
  {
  return sizeof(fd_set);
  }

void log_err(int errnum, const char *routine, const char *text)
  {
  // Do nothing, assume never fail
  }

void log_record(int eventtype, int objclass, const char *objname, const char *text)
  {
  exit(1);
  }

int pbs_getaddrinfo(const char *pNode,struct addrinfo *pHints,struct addrinfo **ppAddrInfoOut)
  {
  return(0);
  }

char *get_cached_nameinfo(const struct sockaddr_in *sai)
  {
  return(NULL);
  }

/* return pointer or NULL */
void *zmq_socket(void *context, int type)
  {
  return g_zmq_socket_ret;
  }

/* return 0 on success or -1 on fail */
int zmq_bind (void *socket, const char *endpoint)
  {
  return g_zmq_bind_ret;
  }

/* return 0 or -1 */
int zmq_close (void *)
  {
  return g_zmq_close_ret;
  }

/* return 0 or -1 */
int add_zconnection(enum zmq_connection_e id, void *socket, void *(*func)(void *), bool should_poll,
    bool connected)
  {
  return g_add_zconnection_ret;
  }

/* return the number of touched items or -1 on error */
int zmq_poll (zmq_pollitem_t *items, int nitems, long timeout)
{
  errno = g_zmq_poll_errno;
  return g_zmq_poll_ret;
}

extern "C" {

/* return max descriptors number */
int get_max_num_descriptors(void)
  {
  return g_get_max_num_descriptors_ret;
  }

/* return 0 if no error */
int pthread_mutex_lock(pthread_mutex_t *mutex) throw()
  {
  g_pthread_mutex_lock_cnt++;
  return 0;
  }

/* return 0 if no error */
int pthread_mutex_unlock(pthread_mutex_t *mutex) throw()
  {
  g_pthread_mutex_unlock_cnt++;
  return 0;
  }

/* return 0 if no error */
int fstat(int fildes, struct stat *buf) throw()
  {
  if (fildes == g_fstat_bad_descriptor)
    {
    return -1;
    }
  return 0;
  }
}
