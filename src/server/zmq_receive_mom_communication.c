#include <pbs_config.h>

#ifdef ZMQ

#include <zmq.h>

#include "log.h"
#include "zmq_process_mom_update.h"

/**
 * Process ZeroMQ MOM status receiving port. Read and process all the received messages.
 * The function blocks while no errors detected.
 * @param zsock ready to be read ZeroMQ socket.
 */
void *start_process_pbs_status_port(
  void *zsock)
  {
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

    if (LOGLEVEL >= 10)
      {
      log_record( PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER, __func__,
          "Processing status port iteration");
      }

    rc = zmq_msg_init(&part);
    if (rc != 0)
      {
      log_err(errno, __func__, "can't init recv message");
      break;
      }
    close_msg = true;

    // Receive the message
    rc = zmq_msg_recv(&part, zsock, 0);
    if (rc == -1)
      {
      log_err(errno, __func__, "can't recv message");
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
      if (msg_part_number == 0 && !more)
        {
        // We expect data block after the ID
        log_err(-1, __func__, "multipart data expected");
        break;
        }

      // Get message data
      size_t sz = zmq_msg_size(&part);
      void *data = zmq_msg_data(&part);
      if (!data)
        {
        log_err(errno, __func__, "can't get message data");
        break;
        }

      // Got well formed message without errors.
      // Process the message
      if (msg_part_number == 0)
        {
        if (LOGLEVEL >= 10)
          {
          sprintf(log_buffer, "ZMQ Message Sender ID: %.*s", (int)sz, (char *)data);
          log_record( PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER, __func__, log_buffer);
          }
        }
      else
        {
        if (LOGLEVEL >= 10)
          {
          sprintf(log_buffer, "ZMQ Message Data: %.*s", (int)sz, (char *)data);
          log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER, __func__, log_buffer);
          }
        rc = pbs_read_json_status(sz, (char *)data);
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

  /* Thread exit */
  return(NULL);
  }



#endif /* ZMQ */
