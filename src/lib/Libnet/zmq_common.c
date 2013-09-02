#include <pbs_config.h>

#ifdef ZMQ

#include <zmq.h>

#include "net_connect.h"
#include "log.h"
#include "pbs_error.h"
#include "zmq_common.h"



/*
 * ZeroMQ related data structures
 */

/**
 * Global pointer to a ZeroMQ context.
 */
void  *g_zmq_context = NULL;

/**
 * Table containing all ZeroMQ connections
 */
struct zconnection_s g_svr_zconn[ZMQ_CONNECTION_COUNT] = {};

/**
 * Pointer to the array containing items to be polled
 */
zmq_pollitem_t *g_zmq_poll_list = NULL;



/**
 * A helper executing the given function in a new thread passing a socket with the given ID to the
 * function as an argument.
 * @param id the ID of a socket argument
 * @param func the function to be executed in a new thread
 */
void start_socket_thread(int id, void*(*func)(void*))
{
  pthread_t thr;
  pthread_create(&thr, NULL, func, g_svr_zconn[id].socket);
}



/**
 * Close ZeroMQ socket immediately, do not wait until any pending data will be sent.
 * @param socket the socket to be closed.
 * @return 0 if operation succeeded or -1 otherwise.
 */
int close_zmq_socket(void *socket)
  {
  const int LINGER = 0;
  int rc;

  if (!socket)
    {
    log_err(-1, __func__, "given socket pointer is NULL");
    return(-1);
    }

  // Set LINGER option to zero. This instruct ZeroMQ that all unsent messages could be dropped now.
  if (zmq_setsockopt(socket, ZMQ_LINGER, &LINGER, sizeof(LINGER)))
    {
    log_err(errno, __func__, "can't set LINGER option to ZMQ socket");
    }

  rc = zmq_close(socket);

  if (rc)
    {
    log_err(errno, __func__, "can't close ZMQ socket");
    }

  return rc;
  }



/**
 * Initialize ZeroMQ related global data structures. Have to be called once per program execution.
 * @return 0 if operation succeeded or -1 otherwise.
 */
int init_zmq()
  {
  g_zmq_poll_list = NULL;
  g_zmq_context = NULL;

  int poll_list_length = get_max_num_descriptors();
  if (poll_list_length <= 0)
    {
    return -1;
    }
  poll_list_length += ZMQ_CONNECTION_COUNT;

  // Allocate the poll array of the max allowed size. This could be optimized in the future.
  g_zmq_poll_list = (zmq_pollitem_t *)calloc(poll_list_length, sizeof(zmq_pollitem_t));
  if (!g_zmq_poll_list)
    {
    log_err(errno, __func__, "can't allocate memory for g_zmq_poll_list array");
    return -1;
    }
 
  // We'll only perform ZMQ_POLLIN poll requests on the list items so init the events here.
  for (int i = 0; i < poll_list_length; i++) {
    g_zmq_poll_list[i].events = ZMQ_POLLIN;
  }

  g_zmq_context = zmq_ctx_new();
  if (!g_zmq_context)
    {
    log_err(errno, __func__, "can't create ZeroMQ context");
    free(g_zmq_poll_list);
    g_zmq_poll_list = NULL;
    return -1;
    }

  return 0;
  }



/**
 * Deinitialize ZeroMQ related global data structures. Have to be called once at the program
 * shutdown.
 */
void deinit_zmq()
  {
  void *socket = NULL;
  int i;

  if (g_zmq_context)
    {
    // Close all sockets
    for (i = 0; i < ZMQ_CONNECTION_COUNT; i++)
      {
      socket = g_svr_zconn[i].socket;
      if (!socket) {
        continue;
      }

      close_zmq_socket(socket);
      g_svr_zconn[i].socket = NULL;
      }

    // Destroy ZMQ context
    zmq_ctx_destroy(g_zmq_context);
    g_zmq_context = NULL;
    }

  // Dealloc global data structures

  if (g_zmq_poll_list)
    {
    free(g_zmq_poll_list);
    g_zmq_poll_list = NULL;
    }
  }



/**
 * Create ZeroMQ client side socket of the specified socket_type and place it to the global
 * connections array. Socket id is the connection index in the array.
 * @param id the connection index in the global array.
 * @param socket_type ZeroMQ socket type.
 * @return 0 if succeeded or -1 otherwise.
 */
int init_zmq_connection(
    enum zmq_connection_e id,
    int  socket_type)

  {
  if (!g_zmq_context || id < 0 || id >= ZMQ_CONNECTION_COUNT)
    {
    log_err(-1, __func__, "wrong arguments specified");
    return -1;
    }

  void *socket = zmq_socket(g_zmq_context, socket_type);
  if (!socket)
    {
    log_err(errno, __func__, "unable to create a socket");
    return -1;
    }

  add_zconnection(id, socket, NULL, false, false);

  return 0;
  }



/**
 * Closes all active connections on the socket with the given id. After this call the socket
 * wouldn't be connected to any endpoint but would still be initialized.
 * @param id the connection index in the global array.
 * @return 0 if succeeded or -1 otherwise.
 */
int close_zmq_connection(
    enum zmq_connection_e id)
  {
  int rc;
  void *socket;
  int socket_type;
  size_t socket_type_length = sizeof(socket_type);

  if (!g_zmq_context || id < 0 || id >= ZMQ_CONNECTION_COUNT)
    {
    log_err(-1, __func__, "wrong arguments specified");
    return(-1);
    }

  if (!g_svr_zconn[id].connected)
    {
    return 0;
    }

  socket = g_svr_zconn[id].socket;
  if (!socket)
    {
    log_err(-1, __func__, "specified socket ID wasn't previously initialized");
    return -1;
    }

  rc = zmq_getsockopt(socket, ZMQ_TYPE, &socket_type, &socket_type_length);
  if (rc)
    {
    log_err(rc, __func__, "unable to get socket option");
    return -1;
    }

  rc = close_zmq_socket(socket);
  if (rc)
    {
    log_err(rc, __func__, "unable to deinit ZMQ socket");
    return -1;
    }
  
  g_svr_zconn[id].socket = NULL;

  socket = zmq_socket(g_zmq_context, socket_type);
  if (!socket)
    {
    log_err(errno, __func__, "unable to create a socket");
    return -1;
    }

  add_zconnection(id, socket, g_svr_zconn[id].func, g_svr_zconn[id].should_poll, false);
  return 0;
  }



/**
 * Add a connection to the global g_svr_zconn ZeroMQ connections array.
 * @param id the connection index in the global array to add to.
 * @param socket ZeroMQ socket pointer to be added.
 * @param func function to invoke on data ready to read.
 * @param should_poll if true the socket would be added to the global poll array and would be polled
 *        for inbound requests.
 * @return 0 if succeed or -1 if wrong id passed.
 */
int add_zconnection(

  enum zmq_connection_e id,
  void *socket,
  void *(*func)(void *),
  bool should_poll,
  bool connected)

  {
  if (id < 0 || id >= ZMQ_CONNECTION_COUNT)
    {
    return -1;
    }

  g_svr_zconn[id].socket = socket;
  g_svr_zconn[id].func   = func;
  g_svr_zconn[id].should_poll = should_poll;
  g_svr_zconn[id].connected = connected;

  return(PBSE_NONE);
  }  /* END add_zconnection() */



/**
 * A handler for processing incoming status messages in Json format on a ZeroMQ socket. Read all
 * messages from the given socket and call the given handler for each message data.
 *
 * If wait argument is set to true the function returns only if an error would be detectd.
 *
 * If wait argument is set to false the function will return when no more messages could be read or
 * if an error detected.
 *
 * The function doesn't check for any error returned by func handler.
 *
 * @param zsock pointer to ZeroMQ socket ready to be read for data.
 * @param func pointer to a data handler.
 * @param wait the socket read operation will be performed in blocking mode if true and non-blocing
 *        if false.
 * @return 0 if no errors 
 */
int process_status_request(

  void *zsock,
  int (*func)(const size_t, const char *),
  bool wait

  )

  {
  if (zsock == NULL || func == NULL)
    {
    log_err(-1, __func__, "wrong input: both socket and function must not be NULL");
    return -1;
    }

  // By 0MQ design the message would consist at least of 2 parts:
  // 1. sender ID
  // 2. first part of the message data
  // By our design messages would consist of the only 2 these parts.
  zmq_msg_t part;
  int msg_part_number = 0;
  bool close_msg = false;
  int more = true;
  size_t more_size = sizeof more;
  int rc = 0;

  // Read all messages received for this time
  while(true)
    {
    rc = zmq_msg_init(&part);
    if (rc != 0)
      {
      log_err(errno, __func__, "can't init recv message");
      break;
      }
    close_msg = true;

    // Receive the message
    rc = zmq_msg_recv(&part, zsock, wait ? 0 : ZMQ_DONTWAIT);
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

    // Process the only first two message parts if multipart. These parts are ID and the message.
    // Process the only first part if non-multipart. The part is the message.
    // Skip other parts if present
    if (msg_part_number < 2)
      {
      // Get message data
      size_t sz = zmq_msg_size(&part);
      char *data = (char *)zmq_msg_data(&part);
      if (!data)
        {
        rc = -1;
        log_err(errno, __func__, "can't get message data");
        break;
        }

      // Got well formed message without errors.
      // Process the message
      if (msg_part_number == 0 && more)
        {
        // First part of a multipart message: client ID
        // Do nothing
        }
      else
        {
        // Non-multipart or second part of a multipart message: data
        func(sz, data);        
        }
      }

    // Deinit the message
    close_msg = false;
    rc = zmq_msg_close(&part);
    if (rc != 0)
      {
      log_err(errno, __func__, "error close ZMQ message");
      break;
      }

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
    int tmp_rc = zmq_msg_close(&part);
    if (tmp_rc != 0)
      {
      log_err(errno, __func__, "error close ZMQ message");
      }
    if (rc == 0)
      {
      rc = tmp_rc;
      }
    }

  return(rc);
  }  /* END process_status_request() */

#endif /* ZMQ */
