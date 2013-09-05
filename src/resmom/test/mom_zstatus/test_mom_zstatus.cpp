#include <stdlib.h>
#include <map>
#include <arpa/inet.h>
#include <assert.h>

#include "../../mom_zstatus.h"
#include "test_mom_zstatus.h"
#include "mom_server.h"

extern int mom_server_count;

extern int                   g_get_max_num_descriptors_ret;
extern int                   g_zmq_close_ret;
extern int                   g_zmq_close_count;
extern int                   g_zmq_connect_ret;
extern std::map<int, int>    g_zmq_connect_ret_map;
extern int                   g_zmq_connect_count;
extern int                   g_zmq_ctx_destroy_ret;
extern void                 *g_zmq_ctx_new_ret;
extern int                   g_zmq_getsockopt_ret;
extern int                   g_zmq_getsockopt_count;
extern bool                  g_zmq_getsockopt_opt_rcvmore;
extern std::map<int, bool>   g_zmq_getsockopt_opt_rcvmore_map;
extern char                 *g_zmq_msg_data_ret;
extern int                   g_zmq_msg_data_count;
extern int                   g_zmq_msg_close_ret;
extern int                   g_zmq_msg_close_count;
extern int                   g_zmq_msg_init_data_ret;
extern int                   g_zmq_msg_init_data_count;
extern std::map<int, int>    g_zmq_msg_init_data_ret_map;
extern int                   g_zmq_msg_send_ret;
extern std::map<int, int>    g_zmq_msg_send_ret_map;
extern int                   g_zmq_msg_send_errno;
extern int                   g_zmq_msg_send_count;
extern int                   g_zmq_msg_send_arg_flags;
extern int                   g_zmq_msg_size_ret;
extern int                   g_zmq_msg_size_count;
extern int                   g_zmq_setsockopt_ret;
extern void                 *g_zmq_socket_ret;
extern std::map<int, void *> g_zmq_socket_ret_map;
extern int                   g_zmq_socket_count;
extern int                   g_MomStatusMessage_deleteString_count;
extern int                   g_MomStatusMessage_readMergeStringStatus_count;
extern int                   g_MomStatusMessage_readMergeJsonStatuses_ret;
extern int                   g_MomStatusMessage_clear_count;

namespace TrqZStatus {

class TestHelper
  {

  public:

    TestHelper(ZStatus *zstatus) : zstatus(zstatus) {}

    void *zsocket()
      {
      return zstatus->zsocket();
      }

    int zconnect(void *socket, struct sockaddr_in *sock_addr, int port)
      {
      return zstatus->zconnect(socket, sock_addr, port);
      }

    int zsend(void *zsocket)
      {
      return zstatus->zsend(zsocket);
      }

    int sendToLevels()
      {
      return zstatus->sendToLevels();
      }

    int connectNextLevel()
      {
      return zstatus->connectNextLevel();
      }

    int connectNodes(resizable_array *nodes)
      {
      return zstatus->connectNodes(nodes);
      }

    int initMsgJsonStatus(zmq_msg_t *message)
      {
      return zstatus->initMsgJsonStatus(message);
      }

    std::vector<void *> *getZmqSocket()
      {
      return &(zstatus->m_zmq_socket);
      }

    void *getZmqServerSocket()
      {
      return zstatus->m_zmq_server_socket;
      }

    int getCPath()
      {
      return zstatus->m_cpath;
      }

    void setCPath(int path)
      {
      zstatus->m_cpath = path;
      }

    int getCLevel()
      {
      return zstatus->m_clevel;
      }
    
    void setCLevel(int level)
      {
      zstatus->m_clevel = level;
      }

  private:

    ZStatus *zstatus;
  };

}

using namespace TrqZStatus;

/**
 * A helper to add a node to a mom hierarcy
 * @param mh the mom hierarchy to be updated
 * @param path the path # the node have to be added to
 * @param level the level # the node have to be added to
 * @param node_name the node name (id)
 * @param addr the node ip address in number-and-dots notation
 * @param port the node port in host byte order
 */
void add_entry(mom_hierarchy_t *mh, int path, int level, char *node_name, char *addr, int port)
  {
  struct sockaddr_in sock_addr_in;
  int rc = inet_aton(addr, &(sock_addr_in.sin_addr));
  assert(rc != 0);

  struct addrinfo mock_addr_info;
  mock_addr_info.ai_addr = (sockaddr *)(&sock_addr_in);

  add_network_entry(mh, node_name, &mock_addr_info, port, path, level);
  }

/**
 * A helper to create mom hierarchy of specified paths, levels and nodes count.
 * The values for would be the following:
 *    node name: node<node#>@lvl<lvl#>.path<path#>
 *    node ip addr: <path#>.0.0.<lvl#>
 *    node port: <node#>
 */
mom_hierarchy_t *create_mh(int paths, int levels, int nodes)
  {
  mom_hierarchy_t *mh;
  mh = initialize_mom_hierarchy();
  for (int path = 0; path < paths; path++)
    {
    for (int level = 0; level < levels; level++)
      {
      /* addr = "123.567.901.345" (len = 15+1) */
      char addr[16];
      snprintf(addr, 16, "%d.0.0.%d", (char)path, (char)level);
      for (int node = 0; node < nodes; node++)
        {
          char node_name[255];
          snprintf(node_name, 255, "node%d@lvl%d.path%d", node, level, path);
          add_entry(mh, path, level, node_name, addr, node);
        }
      }
    }
  return mh;
  }

/**
 * A helper that gets a nodes array from a level of the given hierarchy.
 */
resizable_array *get_nodes(mom_hierarchy_t *mh)
  {
  int cur_path = mh->paths->num;
  resizable_array *levels = (resizable_array *)mh->paths->slots[cur_path].item;
  int cur_level = levels->num;
  resizable_array *nodes  = (resizable_array *)levels->slots[cur_level].item;
  return nodes;
  }

/* Testing function: ZStatus::ZStatus();
 * Input parameters:
 *    mom_hierarchy_t *mh, (original mom hierarchy object)
 *          check: don't fail on any NULL, non-NULL
 *    char *mom_alias, (mom name)
 *          check: don't fail if NULL
 */
START_TEST(zstatus_create_destroy_test)
  {
  /* Check wrong input */
  /* mom_alias is just assigned to a field, so don't check non-null value */
  /* 1. Check both NULL */
  ZStatus *zstatus;
  mom_hierarchy_t *mh;

  zstatus = new ZStatus(NULL, "M");
  delete(zstatus);

  /* 2. Check empty MH */
  /* Mom hierarchy is initialized with initialized paths */
  mh = create_mh(0, 0, 0);

  zstatus = new ZStatus(mh, "M");

  delete(zstatus);
  free_mom_hierarchy(mh);

  /* 3. Check with MH containing one thing */
  mh = create_mh(1, 1, 1);

  zstatus = new ZStatus(mh, "M");

  delete(zstatus);
  free_mom_hierarchy(mh);
  }
END_TEST

/* Testing function: ZStatus::zsocket();
 * Input parameters:
 *    None
 * Output parameters:
 *    void *socket - new ZMQ socket or NULL if failed
 * Used members:
 *    None
 * Used globals:
 *    void *g_zmq_context - ZMQ context
 *        check: nothing, just passed to to zmq_socket
 * Used functions:
 *    zmq_socket(void *, int) - return NULL if fail
 *        check: fail if NULL
 *    zmq_setsockopt(void *, int, void *, size_t) - return 0 if success, -1 if fail
 *        check: close socket and fail if NULL
 *    zmq_close(void *) - return o if success, -1 if fail
 *        check: nothing
 */
START_TEST(zstatus_private_zsocket_test)
  {
  /* Prepare mom hierarchy */
  ZStatus zstatus(NULL, "M");
  TestHelper test_helper(&zstatus);
  void *mock_new_sock = malloc(sizeof(int));
  g_zmq_socket_ret = mock_new_sock;
  g_zmq_setsockopt_ret = 0;
  g_zmq_close_ret = -1;
  void *new_socket;

  /* Check zmq_socket fail */
  g_zmq_socket_ret = NULL;
  new_socket = test_helper.zsocket();
  ck_assert_int_eq(new_socket, NULL);
  g_zmq_socket_ret = mock_new_sock;

  /* Check zmq_setsockopt fail */
  g_zmq_setsockopt_ret = -1;
  new_socket = test_helper.zsocket();
  ck_assert_int_eq(new_socket, NULL);
  ck_assert_int_eq(g_zmq_close_count, 1);
  g_zmq_setsockopt_ret = 0;

  /* Check good case */
  new_socket = test_helper.zsocket();
  ck_assert_int_eq(new_socket, mock_new_sock);

  /* Cleanup test data */
  free(mock_new_sock);
  }
END_TEST

/* Testing function: ZStatus::zconnect();
 * Input parameters:
 *    void *socket - socket to be connected
 *        check: nothing, directly passed to zmq_connect()
 *    struct sockaddr_in *sock_addr - structure containing the remote IP address
 *        check: nothing, used internally without any chance to pass NULL
 *    int port - remote port number in host order
 *        check: sock_addr->sin_port is used if 0, passed value is used if non-0
 * Output parameters:
 *    int - 0 if success or -1 if fail
 * Used members:
 *    None
 * Used globals:
 *    None
 * Used functions:
 *    zmq_connect(void *, char *) - return o if success, -1 if fail
 *        check: fail if -1
 */
START_TEST(zstatus_private_zconnect_test)
  {
  ZStatus zstatus(NULL, "M");
  TestHelper test_helper(&zstatus);
  void *mock_socket = malloc(sizeof(int));
  int rc;

  struct sockaddr_in sock_addr;
  rc = inet_aton("123.123.123.123", &(sock_addr.sin_addr));
  assert(rc != 0);

  g_zmq_connect_ret = 0;
  
  /* Check zmq_connect fail */
  g_zmq_connect_ret = -1;
  rc = test_helper.zconnect(mock_socket, &sock_addr, 0);
  ck_assert_int_eq(rc, -1);
  g_zmq_connect_ret = 0;

  /* Check good case */
  rc = test_helper.zconnect(mock_socket, &sock_addr, 0);
  ck_assert_int_eq(rc, 0);

  /* Cleanup test data */
  free(mock_socket);
  }
END_TEST

/* Testing function: ZStatus::zsend();
 * Input parameters:
 *    void *socket - socket to use for sending
 *        check: nothing it asserts if NULL passed
 * Output parameters:
 *    int - 0 if success or -1 if fail
 * Used members:
 *    None
 * Used globals:
 *    None
 * Used functions:
 *    ZStatus::initMsgJsonStatus(zmq_msg_t *) - initialize the message with collected statuses
 *                                              return -1 if zmq_msg_init_data failed and 0 if success
 *        check: fail if -1
 *    zmq_msg_send(zmq_msg_t *, void *, int) - return -1 if fail, number of sent bytes if success
 *        check: fail if -1
 */
START_TEST(zstatus_private_zsend_test)
  {
  ZStatus zstatus(NULL, "M");
  TestHelper test_helper(&zstatus);
  void *mock_socket = malloc(sizeof(int));
  int rc;

  g_zmq_msg_init_data_ret = 0;
  g_zmq_msg_send_ret = 0;
  
  /* Check initMsgJsonStatus() fail */
  g_zmq_msg_init_data_ret = -1;
  rc = test_helper.zsend(mock_socket);
  ck_assert_int_eq(rc, -1);
  g_zmq_msg_init_data_ret = 0;

  /* Check zmq_msg_send() fail */
  g_zmq_msg_send_ret = -1;
  rc = test_helper.zsend(mock_socket);
  ck_assert_int_eq(rc, -1);
  g_zmq_msg_send_ret = 0;

  /* Check good case */
  rc = test_helper.zsend(mock_socket);
  ck_assert_int_eq(rc, 0);

  /* Cleanup test data */
  free(mock_socket);
  }
END_TEST

/* Testing function: ZStatus::sendToLevels();
 * Input parameters:
 *    None
 * Output parameters:
 *    int - 0 if success or -1 if fail
 * Used members:
 *    std::vector<void *> m_zmq_socket - zmq_sockets vector
 *        check: fail if empty
 * Used globals:
 *    None
 * Used functions:
 *    zsend() - return -1 if fail, 0 if success, will fail if zmq_msg_init_data failed
 *        check: fail if no one return 0
 */
START_TEST(zstatus_private_sendToLevels_test)
  {
  ZStatus zstatus(NULL, "M");
  TestHelper test_helper(&zstatus);
  void *mock_socket = malloc(sizeof(int));
  std::vector<void *> *m_zmq_socket = test_helper.getZmqSocket();
  int rc;

  g_zmq_msg_init_data_ret = 0;
  
  /* Check empty m_zmq_socket */
  rc = test_helper.sendToLevels();
  ck_assert_int_eq(rc, -1);

  /* Set two sockets */
  m_zmq_socket->push_back(mock_socket);
  m_zmq_socket->push_back(mock_socket);

  /* Check all fail */
  g_zmq_msg_init_data_ret_map[1] = -1;
  g_zmq_msg_init_data_ret_map[2] = -1;
  g_zmq_msg_init_data_count = 0;
  rc = test_helper.sendToLevels();
  ck_assert_int_eq(rc, -1);

  /* Check first fail */
  g_zmq_msg_init_data_ret_map[1] = -1;
  g_zmq_msg_init_data_ret_map[2] = 0;
  g_zmq_msg_init_data_count = 0;
  rc = test_helper.sendToLevels();
  ck_assert_int_eq(rc, 0);

  /* Check last fail */
  g_zmq_msg_init_data_ret_map[1] = 0;
  g_zmq_msg_init_data_ret_map[2] = -1;
  g_zmq_msg_init_data_count = 0;
  rc = test_helper.sendToLevels();
  ck_assert_int_eq(rc, 0);

  /* Check none fail */
  g_zmq_msg_init_data_ret_map[1] = 0;
  g_zmq_msg_init_data_ret_map[2] = 0;
  g_zmq_msg_init_data_count = 0;
  rc = test_helper.sendToLevels();
  ck_assert_int_eq(rc, 0);
  }
END_TEST

/* Testing function: ZStatus::connectNextLevel();
 * Input parameters:
 *    None.
 * Output parameters:
 *    int - 0 if success, -1 if fail or -2 if no more levels
 * Used members:
 *    mom_hierarchy_t *m_mh
 *        check: fail if NULL
 *    int m_clevel - current level, 
 *        check: decrease m_cpath, resets m_clevel if 0
 *               uses m_cpath, decrease m_clevel if >0
 *    int m_cpath
 *        check: fail if m_cpath == 1 and m_clevel == 0 (we're on top)
 * Used globals:
 *    None
 * Used functions:
 *    ZStatus::connectNodes() - connects to all given nodes, return 0 if success, -1 if fail
 *                              fails if zmq_socket fail
 *        check: fail if -1
 */
START_TEST(zstatus_private_connectNextLevel_test)
  {
  ZStatus *zstatus;
  TestHelper *test_helper;
  void *mock_socket = malloc(sizeof(int));

  mom_hierarchy_t *mh;
  int rc;

  g_zmq_socket_ret = mock_socket;
  g_zmq_connect_ret = 0;

  /* Check NULL mh */
  zstatus = new ZStatus(NULL, "M");
  test_helper = new TestHelper(zstatus);
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, -2);
  delete(test_helper);
  delete(zstatus);

  /* Create mh with 2 paths, 2 levels and 2 nodes in each */
  mh = create_mh(2, 2, 2);
  zstatus = new ZStatus(mh, "M");
  test_helper = new TestHelper(zstatus);
  /* m_clevel is 1 now, m_cpath is 2 now */
  ck_assert_int_eq(test_helper->getCLevel(), 2);
  ck_assert_int_eq(test_helper->getCPath(), 2);

  /* Check m_clevel > 0, connect error */
  g_zmq_socket_count = 0;
  g_zmq_socket_ret = NULL;
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, -1);
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_socket_count, 2);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getCPath(), 2);

  /* Check m_clevel == 0, m_cpath > 1, connect error */
  g_zmq_socket_count = 0;
  g_zmq_socket_ret = NULL;
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, -1);
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_socket_count, 2);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getCPath(), 1);

  /* Check m_clevel == 0, m_cpath == 1, connect will not called */
  g_zmq_socket_count = 0;
  g_zmq_socket_ret = NULL;
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, -2);
  ck_assert_int_eq(g_zmq_socket_count, 0);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getCPath(), 1);

  /* Check good return */
  test_helper->setCLevel(2);
  test_helper->setCPath(2);
  g_zmq_socket_count = 0;
  g_zmq_socket_ret = mock_socket;
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, 0);
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, 0);
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, 0);
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, 0);
  rc = test_helper->connectNextLevel();
  ck_assert_int_eq(rc, -2);
  ck_assert_int_eq(g_zmq_socket_count, 4);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getCPath(), 1);

  /* Cleanup test resources */
  
  delete(test_helper);
  delete(zstatus);
  }
END_TEST

/* Testing function: ZStatus::connectNodes();
 * Input parameters:
 *    resizable_array *nodes - nodes to connect to
 *        check: fail if empty
 * Output parameters:
 *    int - 0 if success or -1 if fail
 * Used members:
 *    std::vector<void *> m_zmq_socket
 *        check: nothing, just updates it.
 * Used globals:
 *    None
 * Used functions:
 *    ZStatus::zsocket() - create new socket, return NULL if fail, will fail if zmq_socket failed
 *        check: fail if NULL
 *    ZStatus::zconnect() - return -1 if fail, 0 if success, will fail if zmq_connect failed
 *        check: fail if no one returned 0
 */
START_TEST(zstatus_private_connectNodes_test)
  {
  ZStatus zstatus(NULL, "M");
  TestHelper test_helper(&zstatus);
  void *mock_socket = malloc(sizeof(int));
  std::vector<void *> *m_zmq_socket = test_helper.getZmqSocket();

  mom_hierarchy_t *mh;
  resizable_array *nodes;
  int rc;

  g_zmq_socket_ret = mock_socket;
  g_zmq_connect_ret = 0;

  /* Check null inpu nodes array */
  rc = test_helper.connectNodes(nodes);
  ck_assert_int_eq(rc, -1);
  ck_assert(m_zmq_socket->empty());

  /* Check empty input nodes array */
  nodes = initialize_resizable_array(10);
  rc = test_helper.connectNodes(nodes);
  ck_assert_int_eq(rc, -1);
  ck_assert(m_zmq_socket->empty());

  /* Check zsocket fail */
  mh = create_mh(1,1,1);
  nodes = get_nodes(mh);
  g_zmq_socket_ret = NULL;

  rc = test_helper.connectNodes(nodes);
  ck_assert_int_eq(rc, -1);
  ck_assert(m_zmq_socket->empty());

  g_zmq_socket_ret = mock_socket;
  free_mom_hierarchy(mh);

  /* Create 2-nodes list */
  mh = create_mh(1,1,2);
  nodes = get_nodes(mh);

  /* Check all fail */
  g_zmq_connect_ret_map[1] = -1;
  g_zmq_connect_ret_map[2] = -1;
  g_zmq_connect_count = 0;
  rc = test_helper.connectNodes(nodes);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(m_zmq_socket->size(), 1);

  /* Check first fail */
  g_zmq_connect_ret_map[1] = -1;
  g_zmq_connect_ret_map[2] = 0;
  g_zmq_connect_count = 0;
  rc = test_helper.connectNodes(nodes);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(m_zmq_socket->size(), 2);

  /* Check last fail */
  g_zmq_connect_ret_map[1] = 0;
  g_zmq_connect_ret_map[2] = -1;
  g_zmq_connect_count = 0;
  rc = test_helper.connectNodes(nodes);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(m_zmq_socket->size(), 3);

  /* Check none fail */
  /* Check last fail */
  g_zmq_connect_ret_map[1] = 0;
  g_zmq_connect_ret_map[2] = 0;
  g_zmq_connect_count = 0;
  rc = test_helper.connectNodes(nodes);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(m_zmq_socket->size(), 4);

  free_mom_hierarchy(mh);
  }
END_TEST

/* Testing function: ZStatus::initMsgJsonStatus();
 * Input parameters:
 *    zmq_msg_t *message - message to be initialized
 *        check: nothing, used safely
 * Output parameters:
 *    int - 0 if success or -1 if fail
 * Used members:
 *    TrqJson::MomStat::usMessage m_json_status - mom statuses container
 * Used globals:
 *    None
 * Used functions:
 *    zmq_msg_init_data() - return -1 if fail, 0 if success
 *        check: fail if -1
 */
START_TEST(zstatus_private_initMsgJsonStatus_test)
  {
  ZStatus zstatus(NULL, "M");
  TestHelper test_helper(&zstatus);
  zmq_msg_t mock_message;
  int rc;

  g_zmq_msg_init_data_ret = 0;
  
  /* Check zmq_msg_init_data fail */
  /* Check data is deleted if failed */
  g_zmq_msg_init_data_ret = -1;
  g_MomStatusMessage_deleteString_count = 0;
  rc = test_helper.initMsgJsonStatus(&mock_message);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_MomStatusMessage_deleteString_count, 1);
  g_zmq_msg_init_data_ret = 0;

  /* Check good case */
  rc = test_helper.initMsgJsonStatus(&mock_message);
  ck_assert_int_eq(rc, 0);
  }
END_TEST

/* Testing function: ZStatus::sendStatus();
 * Input parameters:
 *    None
 * Output parameters:
 *    int - 0 if success or -1 if fail
 * Used members:
 *    TrqJson::MomStat::usMessage m_json_status - mom statuses container
 * Used globals:
 *    mom_server mom_servers[]
 *    int mom_server_count
 *        check: don't fail if mom_server_count = 0 (no servers)
 * Used functions:
 *    ZStatus::sendToLevels() - send to all connected levels until success, -1 if fail, 0 if success
 *                              fails if zsend returns -1 or if m_zmq_socket is empty
 *        check: do nothing more and return success if 0
 *    ZStatus::connectNextLevel() - return -1 if fail, -2 if no more levles, 0 if success
 *                                  -1 if m_clevel > 0 && m_cpath > 1 && connectNodes() failed
 *                                        connectNodes() fails if zsocket failed
 *                                  -2 if m_clevel == 0 && m_cpath == 1
 *        check: try server if -2, skip zsend if -1, call zsend if 0
 *    ZStatus::zsend() - return -1 if fail or 0 if success
 *                       fails if zmq_msg_init_data failed
 *        check: do nothing more and return success if 0
 *    ZStatus::zsocket() - return NULL if fail or ZMQ socket if success
 *                         fails if zmq_socket failed or zmq_setsockopt failed
 *        check: connect and send if socket created, fail if not.
 *    ZStatus::zconnect() - return -1 if fail or 0 if success.
 *                          fails if zmq_connect failed
 *        check: called for all servers, don't fail if at least one succeed, fail if all failed
 *    close_zmq_socket() - ignoring return
 *        check: called if all zconnect() calls failed
 */
START_TEST(zstatus_public_sendStatus_test)
  {
  ZStatus *zstatus;
  TestHelper *test_helper;
  mom_hierarchy_t *mh;
  void *mock_socket = malloc(sizeof(int));
  int rc;

  struct sockaddr_in *sock_addr_in;

  /* Initialize 2 servers */
  sock_addr_in = (struct sockaddr_in *) calloc(1, sizeof(struct sockaddr_in));
  rc = inet_aton("1.1.1.1", &(sock_addr_in->sin_addr));
  assert(rc != 0);
  mom_servers[0].sock_addr = *sock_addr_in;
  sock_addr_in = (struct sockaddr_in *) calloc(1, sizeof(struct sockaddr_in));
  rc = inet_aton("2.2.2.2", &(sock_addr_in->sin_addr));
  assert(rc != 0);
  mom_servers[1].sock_addr = *sock_addr_in;

  mom_server_count = 2;
  g_zmq_socket_ret = mock_socket;

  /* Test without mh and servers */
  mom_server_count = 0;
  zstatus = new ZStatus(NULL, "M");
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, -1);
  delete(zstatus);

  /* Test with empty mh and servers*/
  mh = create_mh(0, 0, 0);
  zstatus = new ZStatus(mh, "M");
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, -1);
  delete(zstatus);
  free(mh);
  mom_server_count = 2;

  /* Create mom hierarchy of 2 paths, 4 levels, 8 moms */
  mh = create_mh(2, 2, 2);
  zstatus = new ZStatus(mh, "M");
  test_helper = new TestHelper(zstatus);

  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 2);
  ck_assert_int_eq(test_helper->getCLevel(), 2);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 0);
  ck_assert_int_eq(test_helper->getZmqServerSocket(), NULL);

  /* sendToLevels failed: no one level is connected yet. */
  /* connectToNextLevel succeed */
  /* zsend succeed */
  g_zmq_socket_count = 0;
  g_zmq_connect_count = 0;
  g_zmq_msg_send_count = 0;
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_zmq_socket_count, 1);
  ck_assert_int_eq(g_zmq_connect_count, 1 * 2);
  ck_assert_int_eq(g_zmq_msg_send_count, 1);
  
  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 2);
  ck_assert_int_eq(test_helper->getCLevel(), 1);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 1);
  ck_assert_int_eq(test_helper->getZmqServerSocket(), NULL);

  /* sendToLevels succeed, no other is executed */
  g_zmq_socket_count = 0;
  g_zmq_connect_count = 0;
  g_zmq_msg_send_count = 0;
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_zmq_socket_count, 0);
  ck_assert_int_eq(g_zmq_connect_count, 0);
  ck_assert_int_eq(g_zmq_msg_send_count, 1);

  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 2);
  ck_assert_int_eq(test_helper->getCLevel(), 1);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 1);
  ck_assert_int_eq(test_helper->getZmqServerSocket(), NULL);

  /* from here sendToLevels would fail */
  g_zmq_msg_send_ret_map[1] = -1;
  
  /* connectToNextLevel succeed and zsend succed, no other is executed */
  g_zmq_socket_count = 0;
  g_zmq_connect_count = 0;
  g_zmq_msg_send_count = 0;
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_zmq_socket_count, 1);
  ck_assert_int_eq(g_zmq_connect_count, 1 * 2);
  ck_assert_int_eq(g_zmq_msg_send_count, 2);

  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 2);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 2);
  ck_assert_int_eq(test_helper->getZmqServerSocket(), NULL);

  g_zmq_msg_send_ret_map[2] = -1;

  /* connectNextLevel failed with -1 */
  /* connectNextLevel succeed */
  /* zsend failed */
  /* connectNextLevel failed with ERR_NO_LEVEL */
  /* zsocket failed */
  g_zmq_socket_count = 0;
  g_zmq_connect_count = 0;
  g_zmq_msg_send_count = 0;
  g_zmq_connect_ret_map[1] = -1; // Fail connect node 1
  g_zmq_connect_ret_map[2] = -1; // Fail connect node 2
                             // connect node 3 and 4 would succeed
  g_zmq_msg_send_ret_map[3] = -1; // Send 3 failed
  g_zmq_socket_ret_map[3] = NULL; // 2 sockets are created successful by connectNextLevel
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_socket_count, 3);
  ck_assert_int_eq(g_zmq_connect_count, 2 * 2);
  ck_assert_int_eq(g_zmq_msg_send_count, 3);

  g_zmq_socket_ret_map.clear();
  g_zmq_connect_ret_map.clear();

  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 1);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 4);
  ck_assert_int_eq(test_helper->getZmqServerSocket(), NULL);

  g_zmq_msg_send_ret_map[3] = -1;
  g_zmq_msg_send_ret_map[4] = -1;

  /* connectNextLevel failed with ERR_NO_LEVEL */
  /* zsocket succeed */
  /* zconnect failed for all 2 servers */
  g_zmq_socket_count = 0;
  g_zmq_connect_count = 0;
  g_zmq_msg_send_count = 0;
  g_zmq_connect_ret = -1; // Fail connect to servers
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_socket_count, 1); // created server socket
  ck_assert_int_eq(g_zmq_connect_count, 2); // 2 servers
  ck_assert_int_eq(g_zmq_msg_send_count, 4); // 4 sockets for 4 levels
  g_zmq_connect_ret = 0;
  
  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 1);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 4);
  ck_assert_int_eq(test_helper->getZmqServerSocket(), NULL);

  /* connectNextLevel failed with ERR_NO_LEVEL */
  /* zsocket succeed */
  /* zconnect succeed for all 2 servers */
  /* zsend failed for the servers socket */
  g_zmq_socket_count = 0;
  g_zmq_connect_count = 0;
  g_zmq_msg_send_count = 0;
  g_zmq_msg_send_ret_map[5] = -1; // Fail send to servers
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_socket_count, 1);
  ck_assert_int_eq(g_zmq_connect_count, 2); // 2 servers
  ck_assert_int_eq(g_zmq_msg_send_count, 5); // 4 sockets for 4 levels + 1 servers socket
  
  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 1);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 4);
  ck_assert_int_ne(test_helper->getZmqServerSocket(), NULL);

  /* connectNextLevel failed with ERR_NO_LEVEL */
  /* zsocket succeed */
  /* zconnect succeed for all 2 servers */
  /* zsend succeed for the servers socket */
  g_zmq_socket_count = 0;
  g_zmq_connect_count = 0;
  g_zmq_msg_send_count = 0;
  g_zmq_msg_send_ret_map[5] = 0; // Send to servers succeed
  rc = zstatus->sendStatus();
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_zmq_socket_count, 0);
  ck_assert_int_eq(g_zmq_connect_count, 0); // 2 servers
  ck_assert_int_eq(g_zmq_msg_send_count, 5); // 4 sockets for 4 levels + 1 servers socket

  /* check ZStatus state */
  ck_assert_int_eq(test_helper->getCPath(), 1);
  ck_assert_int_eq(test_helper->getCLevel(), 0);
  ck_assert_int_eq(test_helper->getZmqSocket()->size(), 4);
  ck_assert_int_ne(test_helper->getZmqServerSocket(), NULL);

  delete(test_helper);
  delete(zstatus);
  free_mom_hierarchy(mh);
  }
END_TEST

/* Testing function: ZStatus::updateMyJsonStatus();
 * Input parameters:
 *    const char *status_strings - dynamic string with status
 *        check: just passed to m_json_status.readMergeStringStatus
 * Output parameters:
 *    None
 * Used members:
 *    TrqJson::MomStat::usMessage m_json_status - mom statuses container
 *        check: nothing
 *    char *m_mom_alias: mom name
 *        check: nothing
 * Used globals:
 *    None
 * Used functions:
 *    MomJsonStatus::readMergeStringStatus() - return nothing
 *        check: called
 */
START_TEST(zstatus_public_updateMyJsonStatus_test)
  {
  ZStatus zstatus(NULL, "M");

  /* Test */
  g_MomStatusMessage_readMergeStringStatus_count = 0;
  zstatus.updateMyJsonStatus(NULL);
  ck_assert_int_eq(g_MomStatusMessage_readMergeStringStatus_count, 1);
  }
END_TEST

/* Testing function: ZStatus::readStatus();
 * Input parameters:
 *    const size_t sz - message size, just passed to MomJsonStatus::readMergeJsonStatuses()
 *        check: nothing
 *    const char *data - message data, just passed to MomJsonStatus::readMergeJsonStatuses()
 *        check: nothing
 * Output parameters:
 *    int - 0 if success or -1 if fail
 * Used members:
 *    TrqJson::MomStat::usMessage m_json_status - mom statuses container
 * Used globals:
 *    None
 * Used functions:
 *    MomJsonStatus::readMergeJsonStatuses() - return -1 if fail, 0 if success
 *        check: fail if -1
 */
START_TEST(zstatus_public_readStatus_test)
  {
  ZStatus zstatus(NULL, "M");
  TestHelper test_helper(&zstatus);
  int rc;

  /* Test error */
  g_MomStatusMessage_readMergeJsonStatuses_ret = -1;
  rc = zstatus.readStatus(0, NULL);
  ck_assert_int_eq(rc, -1);

  /* Test ok */
  g_MomStatusMessage_readMergeJsonStatuses_ret = 0;
  rc = zstatus.readStatus(0, NULL);
  ck_assert_int_eq(rc, 0);
  }
END_TEST

/* Testing function: ZStatus::clearStatusCache();
 * Input parameters:
 *    None
 * Output parameters:
 *    None
 * Used members:
 *    TrqJson::MomStat::usMessage m_json_status - mom statuses container
 *        check: nothing
 * Used globals:
 *    None
 * Used functions:
 *    MomJsonStatus::clear() - return nothing
 *        check: called
 */
START_TEST(zstatus_public_clearStatusCache_test)
  {
  ZStatus zstatus(NULL, "M");

  /* Test */
  g_MomStatusMessage_clear_count = 0;
  zstatus.clearStatusCache();
  ck_assert_int_eq(g_MomStatusMessage_clear_count, 1);
  }
END_TEST

Suite *zmq_common_suite(void)
  {
  Suite *s = suite_create("mom_zstatus_suite methods");
  TCase *tc_core = tcase_create("zstatus_create_destroy_test");
  tcase_add_test(tc_core, zstatus_create_destroy_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_private_zsocket_test");
  tcase_add_test(tc_core, zstatus_private_zsocket_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_private_zconnect_test");
  tcase_add_test(tc_core, zstatus_private_zconnect_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_private_zsend_test");
  tcase_add_test(tc_core, zstatus_private_zsend_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_private_sendToLevels_test");
  tcase_add_test(tc_core, zstatus_private_sendToLevels_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_private_connectNextLevel_test");
  tcase_add_test(tc_core, zstatus_private_connectNextLevel_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_private_connectNodes_test");
  tcase_add_test(tc_core, zstatus_private_connectNodes_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_private_initMsgJsonStatus_test");
  tcase_add_test(tc_core, zstatus_private_initMsgJsonStatus_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_public_sendStatus_test");
  tcase_add_test(tc_core, zstatus_public_sendStatus_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_public_updateMyJsonStatus_test");
  tcase_add_test(tc_core, zstatus_public_updateMyJsonStatus_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_public_readStatus_test");
  tcase_add_test(tc_core, zstatus_public_readStatus_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("zstatus_public_clearStatusCache_test");
  tcase_add_test(tc_core, zstatus_public_clearStatusCache_test);
  suite_add_tcase(s, tc_core);

  return s;
  }

void rundebug()
  {
  }

int main(void)
  {
  int number_failed = 0;
  SRunner *sr = NULL;
  rundebug();
  sr = srunner_create(zmq_common_suite());
  srunner_set_log(sr, "zmq_common_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  }
