#include <pbs_config.h>

#ifdef ZMQ

#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mom_zstatus.h"
#include "mom_comm.h"
#include "mom_server.h"
#include "../lib/Libnet/zmq_common.h"
#include "mom_config.h"
#include "utils.h"

extern void       *g_zmq_context;
extern mom_server mom_servers[PBS_MAXSERVER];
extern int        mom_server_count;

namespace TrqZStatus 
{
  /**
  * Initialize ZStatus object
  * @param mh MOM hierarchy, could be null if hierarchy was not configured on a server
  * @param mom_alias MOM aliases
  * @return nothing
  */
  ZStatus::ZStatus(mom_hierarchy_t *mh, const char *mom_alias):
    m_mh(mh),
    m_mom_alias(mom_alias),
    m_cpath(0),
    m_clevel(0),
    m_zmq_server_socket(NULL)
  {
    assert(m_mom_alias != NULL && *mom_alias != '\0');
    if (mh
        && mh->paths->num > 0)
    {
      m_cpath = mh->paths->num;
      resizable_array *levels = (resizable_array *)mh->paths->slots[m_cpath].item;
      if (levels->num > 0)
      {
        m_clevel = levels->num;
      }
    }
  }

  /**
  * Deinitialize ZStatus object. Close all openned ZeroMQ sockets
  * @return nothing
  */
  ZStatus::~ZStatus()
  {
    for (int i = 0; i<m_zmq_socket.size(); i++)
    {
      if (m_zmq_socket[i])
      {
        close_zmq_socket(m_zmq_socket[i]);
        m_zmq_socket[i] = NULL;
      }
    }
    m_zmq_socket.clear();

    if (m_zmq_server_socket)
    {
      close_zmq_socket(m_zmq_server_socket);
      m_zmq_server_socket = NULL;
    }
  }

  /**
  * Parse and handle mom status message from the given buffer.
  * @param sz data buffer size.
  * @param data message data buffer.
  * @return 0 if succeeded or -1 otherwise.
  */
  int ZStatus::readStatus(const size_t sz, const char *data)
  {
    return m_json_status.readMergeJsonStatuses(sz, data);
  }

  /**
  * Update this MOM status message in the global statuses map with the values from the given
  * status_strings dynamic string.
  * @param status_strings dynamic string containing this MOM status.
  * @return nothing
  */
  void ZStatus::updateMyJsonStatus(const char *status_strings, bool request_hierarchy)
  {
    m_json_status.readMergeStringStatus(m_mom_alias, status_strings, request_hierarchy);
  }


  /**
  * This function creates new ZeroMQ socket and connects it to the upper level nodes of the current
  * path.
  * If there are no mode levels on the current path it tries to choose the other path.
  * If there are no more paths available return with error
  * @return 0 if succeeded or -1 otherwise.
  */
  int ZStatus::connectNextLevel()
  {
    int ret = ERR_NO_LEVEL;

    if (m_mh
        && m_clevel > 0)
    {
      resizable_array *levels = (resizable_array *)m_mh->paths->slots[m_cpath].item;
      resizable_array *nodes  = (resizable_array *)levels->slots[m_clevel].item;

      /* connect nodes */
      ret = connectNodes(nodes);
      m_clevel--;
    }
    else if (m_mh
             && m_cpath > 1)
    {
      m_cpath--;
      resizable_array *levels = (resizable_array *)m_mh->paths->slots[m_cpath].item;
      m_clevel = levels->num;
      resizable_array *nodes  = (resizable_array *)levels->slots[m_clevel].item;
      m_clevel--;

      /* connect nodes */
      ret = connectNodes(nodes);
    }

    return ret;
  }

  /**
  * This function creates new ZeroMQ socket and connects it to the upper level nodes of the current
  * path.
  * If there are no mode levels on the current path it tries to choose the other path.
  * If there are no more paths available it connects directly to PBS servers.
  * @return 0 if succeeded or -1 otherwise.
  */
  int ZStatus::connectNodes(resizable_array *nodes)
  {
    int ret = -1;

    if (nodes
        && nodes->num)
    {
      /* create new socket */
      void *socket = zsocket();
      if (!socket)
      {
        return -1;
      }

      /* add socket to our connected levels */
      m_zmq_socket.push_back(socket);

      /* shuffle array indices */
      int *idx = (int *)malloc(nodes->num * sizeof(int)); // we don't need 0-ed array here
      shuffle_indices(idx, nodes->num);
  
      /* connect socket to every node on a level */
      ret = -1;
      for (int i = 1; i <= nodes->num; i++)
      {
        node_comm_t *nc = (node_comm_t *)nodes->slots[idx[i]].item;
        /* try to connect, don't fail if at least one was connected */
        if (zconnect(socket, &nc->sock_addr, 0) == 0)
          {
          ret = 0;
          }
      }
    }

    return ret;
  }

  /**
  * Send status messages to upper levels or PBS servers 
  * @return 0 if succeeded or -1 otherwise.
  */
  int ZStatus::sendStatus()
  {
    int ret = -1;
    int i = 0;

    /* Trying to send status to connected levels first */
    ret = sendToLevels();
    while (ret < 0)
    {
      /* connect next level or servers */
      ret = connectNextLevel();
      if (ret == ERR_NO_LEVEL)
      {
        /* no more nodes to connect, break */
        ret = -1;
        break;
      }

      if (ret != 0)
      {
        /* connection error continue with next level */
        continue;
      }

      /* trying to send to the last added level */
      ret = zsend(m_zmq_socket[m_zmq_socket.size() - 1]);
    }

    if (ret < 0)
    {
      /* send status directly to the PBS servers */
      if (!m_zmq_server_socket)
      {
        /* connect to servers first */
        if (mom_server_count)
        {
          /* create new socket */
          m_zmq_server_socket = zsocket();
          if (m_zmq_server_socket)
          {            
            ret = -1;
            /* Connect directly to servers */
            for (int i = 0; i < mom_server_count; i++)
            {
              /* FIXME: use only default server ports now */
              /* try to connect, don't fail if at least one was connected */
              if (zconnect(m_zmq_server_socket, &mom_servers[i].sock_addr, PBS_STATUS_SERVICE_PORT)
                  == 0)
              {
                ret = 0;
              }
            }

            if (ret < 0)
            {
              /* Can not connect to any server close the socket */
              close_zmq_socket(m_zmq_server_socket);
              m_zmq_server_socket = NULL;
            }
          }
        }
      }

      /* send to PBS servers  */
      if (m_zmq_server_socket)
      {
        ret = zsend(m_zmq_server_socket);
      }
    }
    return ret;
  }

  /**
  * Send status messages connected hierarchy levels
  * @return 0 if succeeded or -1 otherwise.
  */
  int ZStatus::sendToLevels()
  {
    int ret = -1;

    for (int i = 0; i < m_zmq_socket.size() && ret < 0; i++)
    {
      ret = zsend(m_zmq_socket[i]);
    }    

    return ret;
  }

  /**
  * Cerate ZeroMQ messages from json statuses
  * @return 0 if succeeded or -1 otherwise.
  */
  int ZStatus::initMsgJsonStatus(zmq_msg_t *message)
  {
    int ret = -1;
    const char *message_data;
    std::string *message_string;

    m_json_status.setMomId(m_mom_alias);
    message_string = m_json_status.write();
    message_data = message_string->c_str();

    ret = zmq_msg_init_data(message, (void *)message_data, message_string->length(),
        m_json_status.deleteString, message_string);
    if (ret)
      {
      /* clean up the data if error */
      m_json_status.deleteString((void *)message_data, (void *)message_string);
      }
    return ret;
  }

  /**
  * Creates and configure new ZeroMQ socket for sending status messages
  * @return pointer to newly created socket or NULL on error
  */
  void *ZStatus::zsocket()
  {
    int hwm = 1;

    /* create new socket */
    void *socket = zmq_socket(g_zmq_context, ZMQ_DEALER);
    if (!socket)
    {
      log_err(errno, __func__, "unable to create a socket");
      return NULL;
    }

    /* set socket options */
    if (zmq_setsockopt(socket, ZMQ_SNDHWM, &hwm, sizeof hwm) != 0)
    {
      log_err(errno, __func__, "unable to set hwm on a socket");
      zmq_close(socket);
      return NULL;
    }

    return socket;
  }

  /**
  * Connects ZeroMQ socket to specified address
  * @param socket ZeroMQ socket
  * @param sock_addr address to connect to
  * @return 0 if succeeded or -1 otherwise.
  */  
  int ZStatus::zconnect(void *socket, struct sockaddr_in *sock_addr, int port)
  {
    #ifndef CONN_URL_LENGTH
      #define CONN_URL_LENGTH 28
    #endif
    char conn_url_buf[CONN_URL_LENGTH]; // Max length: "tcp://123.123.123.123:12345\0" = 28
    size_t printed_length;

    printed_length = snprintf(conn_url_buf, CONN_URL_LENGTH, "tcp://%s:%u",
        inet_ntoa(sock_addr->sin_addr), port ? port : ntohs(sock_addr->sin_port));
    assert(printed_length < CONN_URL_LENGTH);
      
    int ret = zmq_connect(socket, conn_url_buf);
    if (LOGLEVEL >= 10)
    {
      sprintf(log_buffer, "zmq_connect: ret: %d, errno: %d, socket: %p, URL: %s",
          ret, errno, socket, conn_url_buf);
      log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);
    }
    if (ret == -1)
    {
      sprintf(log_buffer, "zmq_connect: can't connect to the server %s", conn_url_buf);
      log_err(errno, __func__, log_buffer);
    }

    return ret;
  }

  /**
  * Sens status message
  * @param socket ZeroMQ socket to send status messages
  * @return 0 if succeeded or -1 otherwise.
  */  
  int ZStatus::zsend(void *socket)
  {
    int ret = 0;

    if (LOGLEVEL >= 9)
    {
      snprintf(log_buffer, sizeof(log_buffer), "Attempting to send status update");
      log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER, __func__, log_buffer);
    }

    if (!socket)
    {
      log_err(-1, __func__, "ZeroMQ socket was not initialized");
      assert(0);
      return -1;
    }

    zmq_msg_t message;
    ret = initMsgJsonStatus(&message);
    if (ret == -1)
      {
      log_err(errno, __func__, "can't initialize ZeroMQ message");
      return -1;
      }

    ret = zmq_msg_send(&message, socket, ZMQ_DONTWAIT);
    if (LOGLEVEL >= 10)
    {
      sprintf(log_buffer, "zmq_msg_send: rc=%d, errno=%d, socket=%p", ret, errno, socket);
      log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER, __func__, log_buffer);
    }

    if (ret != -1)
    {
      if (LOGLEVEL >= 10)
      {
        snprintf(log_buffer, sizeof(log_buffer), "Successfully sent status update");
        log_record(PBSEVENT_SYSTEM, PBS_EVENTCLASS_SERVER,__func__,log_buffer);
      }
    }
    else
    {
      log_err(errno, __func__, "error sending status to the server");
    }

    return ret;
  }

  void ZStatus::clearStatusCache()
  {
    m_json_status.clear();
  }

} /* namespace TrqZStatus */


#endif /* ZMQ */
