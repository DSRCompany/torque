#include <pbs_config.h>

#ifdef ZMQ

#include <zmq.h>
#include <vector>

#include "log.h"
#include "net_connect.h"
#include "mom_hierarchy.h"
#include "MomStatusMessage.hpp"

namespace TrqZStatus 
{
  /**
  * A class to work with status messages
  */
  class ZStatus
  {
  public:
    ZStatus(mom_hierarchy_t *mh, char *mom_alias);
    ~ZStatus();


    int sendStatus();
    void updateMyJsonStatus(char *status_strings);

    int readStatus(size_t sz, char *data);

    void clearStatusCache();

  private:
    TrqJson::MomStatusMessage m_json_status;
    std::vector<void *> m_zmq_socket;
    void *m_zmq_server_socket;

    mom_hierarchy_t *m_mh;
    char *m_mom_alias;

    int m_cpath;
    int m_clevel;
    
    void *zsocket();
    int zconnect(void *socket, struct sockaddr_in *sock_addr, int port);
    int zsend(void *zsocket);

    inline int sendToLevels();
    int connectNextLevel();
    int connectNodes(resizable_array *nodes);

    int initMsgJsonStatus(zmq_msg_t *message);
  };
} /* namespace TrqZStatus */

#endif /* ZMQ */
