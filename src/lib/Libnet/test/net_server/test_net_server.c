#include "pbs_config.h"

#include <stdio.h>

#include "test_net_server.h"
#include "net_connect.h"
#include "lib_net.h"
#include "zmq_common.h"
#include "server_limits.h"

extern fd_set   *GlobalSocketReadSet;
extern u_long   *GlobalSocketAddrSet;
extern u_long   *GlobalSocketPortSet;
extern struct connection svr_conn[];

#include "server_limits.h"
#include "pbs_error.h"

extern void *g_zmq_socket_ret;
extern void *g_zmq_context;
extern int g_zmq_bind_ret;
extern int g_zmq_close_ret;
extern int g_add_zconnection_ret;
extern pthread_mutex_t *global_sock_read_mutex;
extern int g_get_max_num_descriptors_ret;
extern int g_zmq_poll_ret;
extern int g_zmq_poll_errno;
extern int g_fstat_bad_descriptor;
extern int g_pthread_mutex_lock_cnt;
extern int g_pthread_mutex_unlock_cnt;

int add_connection(int sock, enum conn_type type, pbs_net_t addr, unsigned int port, unsigned int socktype, void *(*func)(void *), int add_wait_request);
void *accept_conn(void *new_conn);


START_TEST(netaddr_pbs_net_t_test_one)
  {
  pbs_net_t ipaddr;
  char      *ipaddr_str;

  ipaddr = 167773189;

  ipaddr_str = netaddr_pbs_net_t(ipaddr);
  fail_unless(ipaddr_str != NULL);
  fprintf(stdout, "\n%s\n", ipaddr_str);
  free(ipaddr_str);


  }
END_TEST

START_TEST(netaddr_pbs_net_t_test_two)
  {
  pbs_net_t ipaddr;
  char      *ipaddr_str;

  ipaddr = 0;

  ipaddr_str = netaddr_pbs_net_t(ipaddr);
  fail_unless(!strcmp(ipaddr_str, "unknown"));
  fprintf(stdout, "%s\n", ipaddr_str);
  free(ipaddr_str);


  }
END_TEST

void *mockFunc(void *ptr)
  {
  return ptr;
  }

START_TEST(init_znetwork_test)
  {
  /* Testing function: init_znetwork()
   * Input parameters:
   *    enum zmq_connection_e id, (correct values: 0 - ZMQ_CONNECTION_COUNT-1)
   *          check: -1 if out of range
   *    char *endpoint, (endpoint string in format "proto://ip:port")
   *          check: zmq_bind will fail if incorrect. Don't check
   *    void *(*readfunc)(void *), (callback)
   *          check: -1 if NULL
   *    int  socket_type, (ZMQ socket type)
   *          check: zmq_socket will fail if incorrect. Don't check
   * Global variables:
   *    void *g_zmq_context;
   *          check: -1 if NULL
   * Calling functions:
   *    log_err();
   *          check: assume never fail
   *    void *zmq_socket(void *, int) - return pointer or NULL
   *          check: -1 if NULL
   *    int zmq_bind (void *, const char *); - return 0 on success or -1 on fail
   *          check: -1 if fail
   *    int zmq_close (void *); - return 0 or -1
   *          check: -1 if fail
   *    int add_zconnection(enum zmq_connection_e, void *, void *(*func)(void *), bool, bool) -
   *          return 0 or -1
   *          check: -1 if fail
   */
  void * const mockPtr = malloc(sizeof(int));
  const char *mockEndpoint = "tcp://*:12345";
  enum zmq_connection_e mockId = (enum zmq_connection_e)0;
  int rc;

  /* Check input in good environment */
  g_zmq_context = mockPtr;
  g_zmq_socket_ret = mockPtr;
  g_zmq_bind_ret = 0;
  g_zmq_close_ret = 0;
  g_add_zconnection_ret = 0;

  /* check id parameter */
  rc = init_znetwork(mockId, mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, 0);
  rc = init_znetwork((enum zmq_connection_e)(ZMQ_CONNECTION_COUNT-1), mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, 0);
  rc = init_znetwork(ZMQ_CONNECTION_COUNT, mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, -1);
  rc = init_znetwork((enum zmq_connection_e)-1, mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, -1);

  /* check func parameter */
  rc = init_znetwork(mockId, mockEndpoint, NULL, 0);
  ck_assert_int_eq(rc, -1);

  /* Check in bad environment */
  /* check g_zmq_context bad return */
  g_zmq_context = NULL;
  rc = init_znetwork(mockId, mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, -1);
  g_zmq_context = mockPtr;


START_TEST(test_add_connection)
  {
  fail_unless(add_connection(-1, ToServerDIS, 0, 0, PBS_SOCK_INET, accept_conn, 0) == PBSE_BAD_PARAMETER);
  fail_unless(add_connection(PBS_NET_MAX_CONNECTIONS, ToServerDIS, 0, 0, PBS_SOCK_INET, accept_conn, 0) == PBSE_BAD_PARAMETER);

  /* check zmq_socket bad return */
  g_zmq_socket_ret = NULL;
  rc = init_znetwork(mockId, mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, -1);
  g_zmq_socket_ret = mockPtr;

  /* check zmq_bind bad return */
  g_zmq_bind_ret = -1;
  rc = init_znetwork(mockId, mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, -1);
  g_zmq_bind_ret = 0;

  /* check zmq_close bad return */
  /* DO NOT CHECK: it's called only if bind failed and return and not affect to the return value
   * because it's already -1 */

  /* check add_zconnection bad return */
  g_add_zconnection_ret = -1;
  rc = init_znetwork(mockId, mockEndpoint, mockFunc, 0);
  ck_assert_int_eq(rc, -1);
  g_add_zconnection_ret = 0;

  /* Clear test data */
  free(mockPtr);
  }
END_TEST

START_TEST(wait_zrequest_test)
  {
  /* Testing function: wait_zrequest()
   * Input parameters:
   *    time_t waittime
   *          check: directly passed to zmq_poll. Don't check.
   *    long *SState
   *          check: 1. NULL value, 2. stop the function if the value is changed.
   *          To check it we need to implement multi-thread test with sleep or some synchronization
   *          logic. It's too complex and will be implemented later.
   * Global variables:
   *    pthread_mutex_t *global_sock_read_mutex
   *          check: assume initialized
   *    fd_set *GlobalSocketReadSet
   *          check: assume initialized, check for different states
   *    u_long *GlobalSocketAddrSet
   *          check: assume initialized
   *    u_long *GlobalSocketPortSet
   *          check: assume initialized
   *    zmq_pollitem_t *g_zmq_poll_list;
   *          check: assume allocated
   *    struct zconnection_s g_svr_zconn[ZMQ_CONNECTION_COUNT];
   *          check: assume initialized. Test empty/non-empty
   *    g_svr_zconn[i].func
   *          check: no error in any case (NULL/non-NULL)
   *    struct connection svr_conn[PBS_NET_MAX_CONNECTIONS];
   *          check: assume initialized. Test empty/non-empty
   *    svr_conn[i].func
   *          check: no error in any case (NULL/non-NULL)
   *    svr_conn[i].cn_active
   *          check: Idle/other. Idle connections would be removed from global read, addr and port
   *                 sets
   * Calling functions:
   *    NOTE: Currently wait_zrequest ignores error code returned by pthread_mutex_(lock|unlock) and
   *          it's not obvious what have to be done in the error case. We'll don't check error cases
   *          for these functions for now.
   *    int pthread_mutex_lock(pthread_mutex_t *mutex) - return 0 on success
   *          check: -1 if non-zero
   *    int pthread_mutex_unlock()
   *          check: -1 if non-zero
   *          check both: call count have to be equal, could be checked once after the test.
   *    int get_max_num_descriptors()
   *          check: check -1, 0, >0
   *    void globalset_del_sock(int)
   *          check: don't mock, don't check
   *    void close_conn(int, int)
   *          don't mock. returns nothing would be tested individually.
   *    log_err();
   *          check: assume never fail
   *    zmq_poll:
   *          check: 0 if -1 && errno=EINTR, -1 if -1 && errno!=EINTR, return number of polled items
   *    int fstat(): is used to check the fd is alive.
   *          check: fd is removed from read set if return non-zero
   *    close_conn(int, bool): internal, don't mock
   *          check: fd is removed from read set if called.
   */
  /* Initialize globals */
  void * const mockPtr = malloc(sizeof(int));
  void * const mockSock1 = malloc(sizeof(int));
  void * const mockSock2 = malloc(sizeof(int));
  long mockSState;
  g_get_max_num_descriptors_ret = 512;
  global_sock_read_mutex = (pthread_mutex_t *)mockPtr;
  GlobalSocketReadSet = (fd_set *)calloc(1,sizeof(char) * get_fdset_size());
  GlobalSocketAddrSet = (u_long *)calloc(sizeof(ulong),get_max_num_descriptors());
  GlobalSocketPortSet = (u_long *)calloc(sizeof(ulong),get_max_num_descriptors());
  g_zmq_poll_list = (zmq_pollitem_t *)calloc(get_max_num_descriptors() + ZMQ_CONNECTION_COUNT,
      sizeof(zmq_pollitem_t));
  memset(g_svr_zconn, 0, sizeof(struct zconnection_s) * ZMQ_CONNECTION_COUNT);
  memset(svr_conn, 0, sizeof(struct connection) * PBS_NET_MAX_CONNECTIONS);
  int rc;

  g_zmq_poll_ret = 0;

  /* Check arguments */
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  rc = wait_zrequest(time_t(1), &mockSState);
  ck_assert_int_eq(rc, 0);

  /* Check max_num_descriptors */
  g_get_max_num_descriptors_ret = -1;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, -1);

  g_get_max_num_descriptors_ret = 0;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, -1);

  g_get_max_num_descriptors_ret = 1;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);

  /* Check num descriptors is less than sockets to listen */
  FD_SET(0, GlobalSocketReadSet);
  FD_SET(1, GlobalSocketReadSet);
  FD_SET(5, GlobalSocketReadSet);
  g_get_max_num_descriptors_ret = 1;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);

  /* Check num descriptors is less than sockets to listen and polled one */
  FD_SET(0, GlobalSocketReadSet);
  FD_SET(1, GlobalSocketReadSet);
  FD_SET(5, GlobalSocketReadSet);
  g_zmq_poll_list[0].revents |= ZMQ_POLLIN;
  g_zmq_poll_ret = 1;
  g_get_max_num_descriptors_ret = 1;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  g_zmq_poll_list[0].revents = 0;
  g_zmq_poll_ret = 0;
  
  g_get_max_num_descriptors_ret = 255;

  /* Check a case when we have 0 ZMQ sokets to listen */
  /* 1 socket, read nothing */
  memset(GlobalSocketReadSet, 0, sizeof(char) * get_fdset_size());
  FD_SET(3, GlobalSocketReadSet);
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);

  /* 1 socket, read something */
  FD_SET(3, GlobalSocketReadSet);
  g_zmq_poll_list[0].revents |= ZMQ_POLLIN;
  g_zmq_poll_ret = 1;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  g_zmq_poll_list[0].revents = 0;
  g_zmq_poll_ret = 0;

  /* 3 sockets, read nothing */
  memset(GlobalSocketReadSet, 0, sizeof(char) * get_fdset_size());
  FD_SET(1, GlobalSocketReadSet);
  FD_SET(3, GlobalSocketReadSet);
  FD_SET(4, GlobalSocketReadSet);
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);

  /* 3 socket, read something */
  FD_SET(1, GlobalSocketReadSet);
  FD_SET(3, GlobalSocketReadSet);
  FD_SET(4, GlobalSocketReadSet);
  g_zmq_poll_list[0].revents |= ZMQ_POLLIN;
  g_zmq_poll_ret = 2;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  g_zmq_poll_list[0].revents = 0;
  g_zmq_poll_ret = 0;

  /* Check we have only ZMQ sockets to listen */
  /* 1 ZMQ socket */
  memset(GlobalSocketReadSet, 0, sizeof(char) * get_fdset_size());
  g_svr_zconn[0].socket = mockSock1;
  g_svr_zconn[0].func = mockFunc;
  g_svr_zconn[0].should_poll = true;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  /* 2 ZMQ sockets */
  g_svr_zconn[1].socket = mockSock2;
  g_svr_zconn[1].func = mockFunc;
  g_svr_zconn[1].should_poll = true;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  /* 2 ZMQ sockets and one have data */
  g_zmq_poll_list[0].revents |= ZMQ_POLLIN;
  g_zmq_poll_ret = 1;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  /* 2 ZMQ sockets and 2 have data */
  g_zmq_poll_list[1].revents |= ZMQ_POLLIN;
  g_zmq_poll_ret = 2;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  g_zmq_poll_list[0].revents = 0;
  g_zmq_poll_ret = 0;

  /* Set some fds again to be polled */
  FD_SET(1, GlobalSocketReadSet);
  FD_SET(4, GlobalSocketReadSet);
  FD_SET(5, GlobalSocketReadSet);
  FD_SET(8, GlobalSocketReadSet);
  FD_SET(9, GlobalSocketReadSet);

  /* check zmq_poll */
  g_zmq_poll_ret = -1;
  g_zmq_poll_errno = EINTR;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);

  g_zmq_poll_ret = -1;
  g_zmq_poll_errno = EAGAIN;
  g_fstat_bad_descriptor = 5;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, -1);
  ck_assert(FD_ISSET(1, GlobalSocketReadSet));
  ck_assert(FD_ISSET(4, GlobalSocketReadSet));
  ck_assert(!FD_ISSET(5, GlobalSocketReadSet));
  ck_assert(FD_ISSET(8, GlobalSocketReadSet));
  ck_assert(FD_ISSET(9, GlobalSocketReadSet));
  g_zmq_poll_ret = 0;
  g_fstat_bad_descriptor = 0;

  /* Check incomming data handling (full code coverage after goot zmq_poll) */
  memset(GlobalSocketReadSet, 0, sizeof(char) * get_fdset_size());
  FD_SET(1, GlobalSocketReadSet);
  svr_conn[1].cn_active = Idle;
  FD_SET(4, GlobalSocketReadSet);
  svr_conn[4].cn_active = Primary;
  FD_SET(5, GlobalSocketReadSet);
  svr_conn[5].cn_active = Primary;
  svr_conn[5].cn_func = mockFunc;
  FD_SET(8, GlobalSocketReadSet);
  svr_conn[8].cn_active = FromClientDIS;
  svr_conn[8].cn_func = mockFunc;
  svr_conn[8].cn_lasttime = time((time_t *)NULL);
  FD_SET(9, GlobalSocketReadSet);
  svr_conn[9].cn_active = FromClientDIS;
  svr_conn[9].cn_func = mockFunc;
  svr_conn[9].cn_lasttime = time((time_t *)NULL) - PBS_NET_MAXCONNECTIDLE - 100;
  svr_conn[9].cn_authen |= PBS_NET_CONN_NOTIMEOUT;
  FD_SET(10, GlobalSocketReadSet);
  svr_conn[10].cn_active = FromClientDIS;
  svr_conn[10].cn_func = mockFunc;
  svr_conn[10].cn_lasttime = time((time_t *)NULL) - PBS_NET_MAXCONNECTIDLE - 100;
  svr_conn[10].cn_authen = 0;
  g_svr_zconn[1].func = NULL;
  rc = wait_zrequest(time_t(1), NULL);
  ck_assert_int_eq(rc, 0);
  ck_assert(FD_ISSET(1, GlobalSocketReadSet));
  ck_assert(FD_ISSET(4, GlobalSocketReadSet));
  ck_assert(FD_ISSET(5, GlobalSocketReadSet));
  ck_assert(FD_ISSET(8, GlobalSocketReadSet));
  ck_assert(FD_ISSET(9, GlobalSocketReadSet));
  ck_assert(!FD_ISSET(10, GlobalSocketReadSet));

  /* Check mutex */
  ck_assert_int_eq(g_pthread_mutex_lock_cnt, g_pthread_mutex_unlock_cnt);
  
  /* Clear test data */
  free(mockPtr);
  free(mockSock1);
  free(mockSock2);
  free(GlobalSocketReadSet);
  free(GlobalSocketAddrSet);
  free(GlobalSocketPortSet);
  free(g_zmq_poll_list);
  }
END_TEST

Suite *net_server_suite(void)
  {
  Suite *s = suite_create("net_server_suite methods");
  TCase *tc_core = tcase_create("netaddr_pbs_net_t_test_one");
  tcase_add_test(tc_core, netaddr_pbs_net_t_test_one);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("netaddr_pbs_net_t_test_two");
  tcase_add_test(tc_core, netaddr_pbs_net_t_test_two);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("test_add_connection");
  tcase_add_test(tc_core, test_add_connection);

  tc_core = tcase_create("init_znetwork_test");
  tcase_add_test(tc_core, init_znetwork_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("wait_zrequest_test");
  tcase_add_test(tc_core, wait_zrequest_test);
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
  sr = srunner_create(net_server_suite());
  srunner_set_log(sr, "net_server_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  }
