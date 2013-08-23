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
zmq_pollitem_t *gs_zmq_poll_list = NULL;



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
  // Allocate the poll array of the max allowed size. This could be optimized in the future.
  gs_zmq_poll_list = (zmq_pollitem_t *)calloc(get_max_num_descriptors(), sizeof(zmq_pollitem_t));
  if (!gs_zmq_poll_list)
    {
    log_err(errno, __func__, "can't allocate memory for gs_zmq_poll_list array");
    return -1;
    }
 
  // We'll only perform ZMQ_POLLIN poll requests on the list items so init the events here.
  for (int i = 0; i < get_max_num_descriptors(); i++) {
    gs_zmq_poll_list[i].events = ZMQ_POLLIN;
  }

  g_zmq_context = zmq_ctx_new();
  if (!g_zmq_context)
    {
    log_err(errno, __func__, "can't create ZeroMQ context");
    free(gs_zmq_poll_list);
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
      }

    // Destroy ZMQ context
    zmq_ctx_destroy(g_zmq_context);
    }

  // Dealloc global data structures

  if (gs_zmq_poll_list)
    {
    free(gs_zmq_poll_list);
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
  if (!g_zmq_context || id >= ZMQ_CONNECTION_COUNT)
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

  if (!g_zmq_context || id >= ZMQ_CONNECTION_COUNT)
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
  if (id >= ZMQ_CONNECTION_COUNT)
    {
    return -1;
    }

  g_svr_zconn[id].socket = socket;
  g_svr_zconn[id].func   = func;
  g_svr_zconn[id].should_poll = should_poll;
  g_svr_zconn[id].connected = false;

  return(PBSE_NONE);
  }  /* END add_zconnection() */

#endif /* ZMQ */
