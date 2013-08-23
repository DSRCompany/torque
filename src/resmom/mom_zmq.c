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
#include "lib_net.h"
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



/**
 * Close the ZeroMQ status messages sending socket. Open it again and connect to all up-level MOMs
 * or to the PBS server if no MOM hierarchy received yet or if all MOMs down.
 * @return PBSE_NONE (0) if succeeded,
 *         NO_SERVER_CONFIGURED if no one server to connect found or
 *         COULD_NOT_CONTACT_SERVER if all connection attempts failed.
 */
int update_status_connection()
  {
  int rc = PBSE_NONE;
  node_comm_t *nc = NULL;
  bool connected = false;

  if (LOGLEVEL >= 9)
    {
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, "Update status port connections");
    }

  close_zmq_connection(ZMQ_STATUS_SEND);
  zmq_setopt_hwm(ZMQ_STATUS_SEND, 1);

  // Try to connect to all nodes of a level.
  nc = update_current_path(mh);
  if (nc != NULL)
    {
    node_comm_t *first_node= nc;
    do
      {
      if (LOGLEVEL >= 9)
        {
        sprintf(log_buffer, "Connecting to node %s", nc->name);
        log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);
        }
      // FIXME: Temporary workaround for the non-standard connection port numbers:
      //        Calculate the ZMQ port number as generic tcp socket port plus difference between
      //        predefined values.
      //        In the future it have to be configurable as it done for generic tcp sockets ports.
      unsigned int port = ntohs(nc->sock_addr.sin_port)
        + (PBS_MOM_STATUS_SERVICE_PORT - PBS_MANAGER_SERVICE_PORT);
      rc = zmq_connect_sockaddr(ZMQ_STATUS_SEND, &(nc->sock_addr), port);
      if (rc != -1) // Success
        {
        connected = true;
        }
      nc = update_current_path(mh);
      }
    while (nc && nc != first_node);
    }

  rc = PBSE_NONE;

  // If connected to no one node try to connect to one of the servers.
  if (!connected)
    {
    int servers_tried = 0;
    /* now, once we contact one server we stop attempting to report in */
    for (int i = 0; i < PBS_MAXSERVER; i++)
      {
      if ((mom_servers[i].pbs_servername[0] == '\0') ||
          (time_now < (mom_servers[i].MOMLastSendToServerTime + ServerStatUpdateInterval)))
        {
        continue;
        }
      if (LOGLEVEL >= 9)
        {
        sprintf(log_buffer, "Connecting to pbs server %s", mom_servers[i].pbs_servername);
        log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);
        }
      // FIXME: Temporary workaround for the non-standard connection port numbers:
      //        Calculate the ZMQ port number as generic tcp socket port plus difference between
      //        predefined values.
      //        In the future it have to be configurable as it done for generic tcp sockets ports.
      unsigned int port = ntohs(mom_servers[i].sock_addr.sin_port)
        + (PBS_STATUS_SERVICE_PORT - PBS_BATCH_SERVICE_PORT);
      rc = zmq_connect_sockaddr(ZMQ_STATUS_SEND, &(mom_servers[i].sock_addr), port);
      if (rc != -1) // Success
        {
        connected = true;
        }
      servers_tried++;
      }
    if (!connected)
      {
      if (servers_tried > 0) // Error
        {
        log_err(-1, __func__, "Could not contact any of the servers to send an update");
        rc = COULD_NOT_CONTACT_SERVER;
        }
      else // Not connected but no one error detected
        {
        rc = NO_SERVER_CONFIGURED;
        }
      }
    }

  return rc;
  }

#endif /* ZMQ */
