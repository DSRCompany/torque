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
  /* Friend class for unit testing */
  friend class TestHelper;

  public:
    ZStatus(mom_hierarchy_t *mh, const char *mom_alias);
    ~ZStatus();


    int sendStatus();
    void updateMyJsonStatus(boost::ptr_vector<std::string> mom_status, bool request_hierarchy);

    int readStatus(const size_t sz, const char *data);

    void clearStatusCache();

  private:
    enum Errors
    {
      ERR_OK = 0,
      ERR_FAIL = -1,
      ERR_NO_LEVEL = -2
    };

    TrqJson::MomStatusMessage m_json_status;
    std::vector<void *> m_zmq_socket;
    void *m_zmq_server_socket;

    mom_hierarchy_t *m_mh;
    const char *m_mom_alias;

    int m_cpath;
    int m_clevel;
    
    void *zsocket();
    int zconnect(void *socket, struct sockaddr_in *sock_addr, int port);
    int zsend(void *zsocket);

    int sendToLevels();
    int connectNextLevel();
    int connectNodes(resizable_array *nodes);

    int initMsgJsonStatus(zmq_msg_t *message);
  };
} /* namespace TrqZStatus */

#endif /* ZMQ */
