#include <pbs_config.h>

#ifdef ZMQ

#include <zmq.h>

#include "log.h"
#include "zmq_process_status.h"



/**
 * A handler for processing incoming status messages in Json format on a ZeroMQ socket.
 * @param args handler arguments list (void **). For this handler the list have to contain a pointer
 * to the ZeroMQ socket to be read for data.
 */
void *status_request(

  void *args)

  {
  void *zsock = *(void **)args;

  // By 0MQ design the message would consist at least of 2 parts:
  // 1. sender ID
  // 2. first part of the message data
  // By our design messages would consist of the only 2 these parts.
  zmq_msg_t part;
  int msg_part_number = 0;
  bool close_msg = false;
  int more = true;
  size_t more_size = sizeof more;
  // Read all messages received for this time
  while(true)
    {
    int rc = 0;

    rc = zmq_msg_init(&part);
    if (rc != 0)
      {
      log_err(errno, __func__, "can't init recv message");
      break;
      }
    close_msg = true;

    // Receive the message
    rc = zmq_msg_recv(&part, zsock, ZMQ_DONTWAIT);
    if (rc == -1)
      {
      // errno = EAGAIN means there is no message to receive
      if (errno == EAGAIN)
        {
        rc = 0;
        }
      else
        {
        log_err(errno, __func__, "can't recv message");
        }
      break;
      }

    // Check if there are more part(s)
    rc = zmq_getsockopt(zsock, ZMQ_RCVMORE, &more, &more_size);
    if (rc != 0)
      {
      log_err(errno, __func__, "can't get socket option");
      break;
      }

    // Process the only first two message parts that are ID and the message.
    // Skip other parts if present
    if (msg_part_number < 2)
      {
      // Get message data
      size_t sz = zmq_msg_size(&part);
      char *data = (char *)zmq_msg_data(&part);
      if (!data)
        {
        log_err(errno, __func__, "can't get message data");
        break;
        }

      // Got well formed message without errors.
      // Process the message
      if (msg_part_number == 0 && more)
        {
        // First part of a multipart message: client ID
        if (LOGLEVEL >= 10)
          {
          sprintf(log_buffer, "ZMQ Message Sender ID: %.*s", (int)sz, (char *)data);
          log_record( PBSEVENT_SYSTEM, PBS_EVENTCLASS_NODE, __func__, log_buffer);
          }
        }
      else
        {
        // Non-multipart or second part of a multipart message: data
        if (LOGLEVEL >= 10)
          {
          sprintf(log_buffer, "ZMQ Message Data: %.*s", (int)sz, (char *)data);
          log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_NODE, __func__, log_buffer);
          }
        mom_read_json_status(sz, data);
        }
      }

    // Deinit the message
    close_msg = false;
    zmq_msg_close(&part);

    // Update part number
    if (more)
      {
      msg_part_number++;
      }
    else
      {
      msg_part_number = 0;
      }
    }
  // Cleanup after break
  if (close_msg)
    {
    zmq_msg_close(&part);
    }

  return(NULL);
  }  /* END status_request() */

#endif /* ZMQ */
