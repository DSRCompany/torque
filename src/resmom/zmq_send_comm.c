#include <pbs_config.h>

#ifdef ZMQ

#include <zmq.h>

#include "log.h"
#include "net_connect.h"
#include "zmq_process_status.c"



extern struct zconnection_s g_svr_zconn[];



/**
 * Send given character buffer to the status messaging ZeroMQ socket.
 * @param status_string the character buffer to be sent.
 * @return 0 if succeeded or -1 otherwise.
 */
int zmq_send_status(
 
  char *status_string)
 
  {
  int rc;
  void *zsocket;

  if (LOGLEVEL >= 9)
    {
    snprintf(log_buffer, sizeof(log_buffer),
      "Attempting to send status update");
    log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER, __func__, log_buffer);
    }
 
  zsocket = g_svr_zconn[ZMQ_STATUS_SEND].socket;
  if (!zsocket)
    {
    log_err(-1, __func__, "ZeroMQ socket isn't initialized yet");
    return(-1);
    }

  zmq_msg_t message;
  init_msg_json_status(&message);

  sprintf(log_buffer, "PID: %u", getpid());
  log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);
  rc = zmq_msg_send(&message, zsocket, ZMQ_DONTWAIT);
  if (LOGLEVEL >= 10)
    {
    sprintf(log_buffer, "zmq_msg_send: rc=%d, errno=%d, socket=%p", rc, errno, zsocket);
    log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER, __func__, log_buffer);
    }

  if (rc != -1)
    {
    if (LOGLEVEL >= 10)
      {
      snprintf(log_buffer, sizeof(log_buffer),
          "Successfully sent status update");
      log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER,__func__,log_buffer);
      }
    }
  else
    {
    log_err(errno, __func__, "error sending status to the server");
    }

  zmq_msg_close(&message);

  return(rc == -1 ? -1 : 0);
  } /* END zmq_send_status() */

#endif /* ZMQ */



