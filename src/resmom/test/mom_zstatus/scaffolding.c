#include "pbs_config.h"
#include "license_pbs.h" /* See here for the software license */

#include <map>
#include <zmq.h>
#include <stdlib.h>

#include "MomStatusMessage.hpp"
#include "mom_server.h"
#include "resmon.h"
#include "log.h"
#include "mom_hierarchy.h"


void          *g_zmq_context;
mom_server     mom_servers[PBS_MAXSERVER];
int            mom_server_count = 0;
int                     LOGLEVEL = 10;
char log_buffer[LOG_BUF_SIZE];

int   g_MomStatusMessage_readMergeJsonStatuses_ret;
int   g_MomStatusMessage_readMergeStringStatus_count;
int   g_MomStatusMessage_clear_count;
int   g_MomStatusMessage_deleteString_count;
int   g_zmq_msg_init_data_ret;
std::map<int, int>    g_zmq_msg_init_data_ret_map;
int g_zmq_msg_init_data_count;
int   g_zmq_msg_send_ret;
std::map<int, int>    g_zmq_msg_send_ret_map;
int   g_zmq_msg_send_count;
int   g_zmq_connect_ret;
std::map<int, int>    g_zmq_connect_ret_map;
int   g_zmq_connect_count;
int   g_zmq_setsockopt_ret;
int                 g_zmq_close_ret;
int                 g_zmq_close_count;
void               *g_zmq_socket_ret;
std::map<int, void *>    g_zmq_socket_ret_map;
int                 g_zmq_socket_count;

const char **dis_emsg;

mom_hierarchy_t *mh;
int  mom_hierarchy_retry_time;

#if 0
int                 g_get_max_num_descriptors_ret;
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


void log_err(int errnum, const char *routine, const char *text)
  {
  // Do nothing, assume never fail
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

extern "C" {

/* return max descriptors number */
int get_max_num_descriptors(void)
  {
  return g_get_max_num_descriptors_ret;
  }

}
#endif



namespace TrqJson {

void Message::setMomId(std::string momId)
  {
  this->momId = momId;
  }

std::string *Message::write()
  {
  std::string *out = new std::string();
  return out;
  }

void Message::deleteString(void *string_data, void *string)
  {
  g_MomStatusMessage_deleteString_count++;
  (void)string_data; /* avoid 'unused' compiler warning */
  if (string == NULL)
    {
    return;
    }
  delete((std::string *)string);
  }

int MomStatusMessage::readMergeJsonStatuses(const size_t size, const char *data)
  {
  return g_MomStatusMessage_readMergeJsonStatuses_ret;
  }

void MomStatusMessage::readMergeStringStatus(const char *nodeId, boost::ptr_vector<std::string> mom_status, bool request_hierarchy)
  {
  g_MomStatusMessage_readMergeStringStatus_count++;
  }

void MomStatusMessage::clear()
  {
  g_MomStatusMessage_clear_count++;
  }

void MomStatusMessage::generateBody(Json::Value &messageBody)
  {
  }

std::string MomStatusMessage::getMessageType()
  {
  return "";
  }

} /* namespace TrqJson */

/* return 0 or -1 */
int zmq_msg_init_data(zmq_msg_t *msg, void *data, size_t size, zmq_free_fn *ffn, void *hint)
  {
  g_zmq_msg_init_data_count++;
  if (g_zmq_msg_init_data_ret_map.count(g_zmq_msg_init_data_count))
    {
    return g_zmq_msg_init_data_ret_map[g_zmq_msg_init_data_count];
    }
  return g_zmq_msg_init_data_ret;
  }

int zmq_msg_send(zmq_msg_t *msg, void *s, int flags)
  {
  g_zmq_msg_send_count++;
  if (g_zmq_msg_send_ret_map.count(g_zmq_msg_send_count))
    {
    return g_zmq_msg_send_ret_map[g_zmq_msg_send_count];
    }
  return g_zmq_msg_send_ret;
  }

int zmq_connect (void *s, const char *addr)
  {
  g_zmq_connect_count++;
  if (g_zmq_connect_ret_map.count(g_zmq_connect_count))
    {
    return g_zmq_connect_ret_map[g_zmq_connect_count];
    }
  return g_zmq_connect_ret;
  }

/* return 0 or -1 */
int zmq_setsockopt(void *s, int option, const void *optval, size_t optvallen)
  {
  return g_zmq_setsockopt_ret;
  }

/* return 0 or -1 */
int zmq_close(void *socket)
  {
  g_zmq_close_count++;
  return g_zmq_close_ret;
  }

/* return pointer or NULL */
void *zmq_socket(void *context, int type)
  {
  g_zmq_socket_count++;
  if (g_zmq_socket_ret_map.count(g_zmq_socket_count))
    {
    return g_zmq_socket_ret_map[g_zmq_socket_count];
    }
  return g_zmq_socket_ret;
  }

int close_zmq_socket(void *socket)
  {
  return 0;
  }

int socket_get_tcp_priv()
  {
  fprintf(stderr, "The call to socket_get_tcp_priv needs to be mocked!!\n");
  exit(1);
  }

int DIS_tcp_wflush(struct tcp_chan *chan)
  {
  fprintf(stderr, "The call to DIS_tcp_wflush needs to be mocked!!\n");
  exit(1);
  }

void log_event(int, int, char const*, char const*)
  {
  // Do nothing, assume never fail
  }

void log_record(int, int, char const*, char const*)
  {
  // Do nothing, assume never fail
  }

void log_err(int errnum, const char *routine, const char *text)
  {
  // Do nothing, assume never fail
  }

long disrsl(struct tcp_chan *chan, int *retval)
  {
  fprintf(stderr, "The call to disrsl needs to be mocked!!\n");
  exit(1);
  }

int diswsl(struct tcp_chan *chan, long value)
  {
  fprintf(stderr, "The call to diswsl needs to be mocked!!\n");
  exit(1);
  }

int socket_connect_addr(
  int              *socket,
  struct sockaddr  *remote,
  size_t            remote_size,
  int               is_privileged,
  char            **error_msg)
  {
  fprintf(stderr, "The call to diswsl needs to be mocked!!\n");
  exit(1);
  }

int pbs_getaddrinfo(const char *hostname, struct addrinfo *in, struct addrinfo **out)
  {
  return(0);
  }

ssize_t read_ac_socket(int fd, void *buf, ssize_t count)
  { 
  fprintf(stderr, "The call to read_nonblocking_socket needs to be mocked!!\n");
  exit(1);
  }

ssize_t write_ac_socket(int fd, const void *buf, ssize_t count)
  {
  return(0);
  }

int get_parent_and_child(char *start, char **parent, char **child, char **end)
  {
  fprintf(stderr, "The call to get_parent_and_child needs to be mocked!!\n");
  exit(1);
  }
