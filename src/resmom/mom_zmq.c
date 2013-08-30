#include <pbs_config.h>

#ifdef ZMQ
#include <zmq.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include "net_connect.h"
#include "log.h"
#include "pbs_error.h"
#include "mom_hierarchy.h"
#include "pbs_ifl.h"
#include "resmon.h"
#include "mom_server.h"
#include "../lib/Libnet/zmq_common.h"



extern struct zconnection_s g_svr_zconn[];
extern mom_hierarchy_t *mh;
extern mom_server mom_servers[];
extern time_t time_now;
extern int ServerStatUpdateInterval;



/**
 * Connect the socket with the specified id using the ip address and the port from the given
 * sock_addr structure.
 * @param id the array index of the socket to be connected.
 * @param sock_addr the sockattr_in structure containing IP the socket to be connected to.
 * @param port the TCP port to connect to.
 * @return 0 if succeeded or -1 otherwise.
 */
int zmq_connect_sockaddr(enum zmq_connection_e id, struct sockaddr_in *sock_addr, int port)
  {
// TODO: Rework this:
// 1. We have to use different port (probably default for now)
// 2. Move the define to appropriate header file
#ifndef CONN_URL_LENGTH
#define CONN_URL_LENGTH 28
#endif
  char conn_url_buf[CONN_URL_LENGTH]; // Max length: "tcp://123.123.123.123:12345\0" = 28
  size_t printed_length;
  int rc;

  printed_length = snprintf(conn_url_buf, CONN_URL_LENGTH,
      "tcp://%s:%u", inet_ntoa(sock_addr->sin_addr), port);
  assert(printed_length < CONN_URL_LENGTH);

  void *zsocket = g_svr_zconn[id].socket;
  rc = zmq_connect(zsocket, conn_url_buf);
  if (LOGLEVEL >= 10)
    {
    sprintf(log_buffer, "connected: rc: %d, errno: %d, socket: %p, URL: %s", rc, errno, zsocket, conn_url_buf);
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);
    }
  if (rc == -1)
    {
    sprintf(log_buffer, "can't connect to the server %s", conn_url_buf);
    log_err(errno, __func__, log_buffer);
    }
  else
    {
    if (LOGLEVEL >= 10)
      {
      sprintf(log_buffer, "connected to the server %s", conn_url_buf);
      log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);
      }
    g_svr_zconn[id].connected = true;
    }

  return(rc);
  }



/**
 * Set SNDHWM and RCVHWM ZeroMQ socket options to the specified value.
 * @param id the socket array index.
 * @param HWM value.
 * @return 0 if succeeded or -1 otherwise.
 */
int zmq_setopt_hwm(zmq_connection_e id, int value)
  {
  const size_t value_len = sizeof(value);

  int rc = zmq_setsockopt(g_svr_zconn[ZMQ_STATUS_SEND].socket, ZMQ_SNDHWM, &value, value_len);
  if (rc != 0)
    {
    rc = zmq_setsockopt(g_svr_zconn[ZMQ_STATUS_SEND].socket, ZMQ_RCVHWM, &value, value_len);
    }
  return rc;
  }

#endif /* ZMQ */
