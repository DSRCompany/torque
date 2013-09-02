#include <stdlib.h>
#include <map>

#include "zmq_common.h"
#include "test_zmq_common.h"

extern int                 g_get_max_num_descriptors_ret;
extern int                 g_zmq_close_ret;
extern int                 g_zmq_ctx_destroy_ret;
extern void               *g_zmq_ctx_new_ret;
extern int                 g_zmq_getsockopt_ret;
extern int                 g_zmq_getsockopt_count;
extern bool                g_zmq_getsockopt_opt_rcvmore;
extern std::map<int, bool> g_zmq_getsockopt_opt_rcvmore_map;
extern char               *g_zmq_msg_data_ret;
extern int                 g_zmq_msg_data_count;
extern int                 g_zmq_msg_close_ret;
extern int                 g_zmq_msg_close_count;
extern int                 g_zmq_msg_init_ret;
extern int                 g_zmq_msg_init_count;
extern int                 g_zmq_msg_recv_ret;
extern std::map<int, int>  g_zmq_msg_recv_ret_map;
extern int                 g_zmq_msg_recv_errno;
extern int                 g_zmq_msg_recv_count;
extern int                 g_zmq_msg_recv_arg_flags;
extern int                 g_zmq_msg_size_ret;
extern int                 g_zmq_msg_size_count;
extern int                 g_zmq_setsockopt_ret;
extern void               *g_zmq_socket_ret;

int gs_data_processor_count;

void *mockFunc(void *ptr)
  {
  return ptr;
  }

int mock_data_processor(const size_t sz, const char *data)
  {
  gs_data_processor_count++;
  return -1;
  }

START_TEST(close_zmq_socket_test)
  {
  /* Testing function: close_zmq_socket()
   * Input parameters:
   *    void *socket, (ZMQ socket)
   *          check: -1 if NULL, ZMQ requests will fail if incorrect.
   * Output parameters:
   *    int, (ret code)
   *          check: -1 if fail, 0 if success.
   * Calling functions:
   *    log_err();
   *          check: assume never fail
   *    int zmq_setsockopt(void *, int, const void *, size_t) - return 0 if OK or -1 if fail
   *          check: 0 even if fail
   *    int zmq_close (void *); - return 0 or -1
   *          check: -1 if fail
   */
  void *mock_ptr = malloc(sizeof(int));
  g_zmq_setsockopt_ret = 0;
  g_zmq_close_ret = 0;
  void *socket = mock_ptr;
  int rc = 0;

  /* Check wrong input */
  socket = NULL;
  rc = close_zmq_socket(socket);
  ck_assert_int_eq(rc, -1);
  socket = mock_ptr;

  /* Check return if zmq_setsockopt fail */
  g_zmq_setsockopt_ret = -1;
  rc = close_zmq_socket(socket);
  ck_assert_int_eq(rc, 0);
  g_zmq_setsockopt_ret = 0;

  /* Check return if zmq_close fail */
  g_zmq_close_ret = -1;

  rc = close_zmq_socket(socket);
  ck_assert_int_eq(rc, -1);

  g_zmq_setsockopt_ret = -1;
  rc = close_zmq_socket(socket);
  ck_assert_int_eq(rc, -1);
  g_zmq_setsockopt_ret = 0;

  g_zmq_close_ret = 0;

  /* Cleanup test data */
  free(mock_ptr);
  }
END_TEST

START_TEST(init_zmq_test)
  {
  /* Testing function: init_zmq()
   * Input parameters:
   *    NONE
   * Output parameters:
   *    int, (return code)
   *          check: -1 if fail, 0 if success.
   * Global variables:
   *    zmq_pollitem_t *g_zmq_poll_list
   *          check: allocated if success, each item events field is set to ZMQ_POLLIN
   *                 NULL-ed if fail
   *    void *g_zmq_context
   *          check: initialized if success, 
   *                 NULL-ed if fail
   * Calling functions:
   *    log_err();
   *          check: assume never fail
   *    int get_max_num_descriptors()
   *          check: fail if <= 0
   *    void *zmq_ctx_new()
   *          check: -1 if fail, all resources cleaned up.
   */
  /* Initialize globals */
  zmq_pollitem_t *mock_poll = (zmq_pollitem_t *) malloc(sizeof(zmq_pollitem_t));
  void *mock_ptr = malloc(sizeof(int));
  void *mock_ptr2 = malloc(sizeof(int));
  g_zmq_poll_list = mock_poll;
  g_zmq_context = mock_ptr;
  g_get_max_num_descriptors_ret = 2;
  g_zmq_ctx_new_ret = mock_ptr2;
  int rc = 0;

  /* Check get_max_num_descriptors() */
  g_get_max_num_descriptors_ret = -1;
  rc = init_zmq();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, NULL);
  g_zmq_poll_list = mock_poll;
  g_zmq_context = mock_ptr;

  g_get_max_num_descriptors_ret = 0;
  rc = init_zmq();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, NULL);
  g_zmq_poll_list = mock_poll;
  g_zmq_context = mock_ptr;
  g_get_max_num_descriptors_ret = 2;

  /* Check zmq_ctx_new */
  g_zmq_ctx_new_ret = NULL;
  rc = init_zmq();
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, NULL);
  g_zmq_poll_list = mock_poll;
  g_zmq_context = mock_ptr;
  g_zmq_ctx_new_ret = mock_ptr2;

  /* Check good case */
  rc = init_zmq();
  ck_assert_int_eq(rc, 0);
  ck_assert_int_ne(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, mock_ptr2);
  for (int i = 0; i < g_get_max_num_descriptors_ret + ZMQ_CONNECTION_COUNT; i++)
    {
    ck_assert_int_eq(g_zmq_poll_list[i].events, ZMQ_POLLIN);
    }

  /* Cleanup test data */
  free(mock_poll);
  free(mock_ptr);
  free(mock_ptr2);
  }
END_TEST

START_TEST(deinit_zmq_test)
  {
  /* Testing function: deinit_zmq()
   * Input parameters:
   *    NONE
   * Output parameters:
   *    NONE, (assume called once at the server shutdown)
   * Global variables:
   *    void *g_zmq_context
   *          check: do not fail if NULL,
   *                 NULL-ed at the end
   *    struct zconnection_s g_svr_zconn[ZMQ_CONNECTION_COUNT]
   *          check: close all non-null sockets
   *                 do not fail on NULL sockets
   *    zmq_pollitem_t *g_zmq_poll_list
   *          check: do not fail if NULL,
   *                 NULL-ed at the end
   * Calling functions:
   *    int close_zmq_socket(void *)
   *          check: Nothing
   *    int zmq_ctx_destroy()
   *          check: Nothing
   */
  /* Initialize globals */
  zmq_pollitem_t *mock_poll = (zmq_pollitem_t *) malloc(sizeof(zmq_pollitem_t));
  void *mock_ptr = malloc(sizeof(int));
  g_zmq_context = mock_ptr;
  g_zmq_poll_list = mock_poll;
  for (int i = 0; i < ZMQ_CONNECTION_COUNT; i++)
    {
    g_svr_zconn[i].socket = mock_ptr;
    }
  g_zmq_ctx_destroy_ret = -1;

  /* Check ctx and poll list are NULL */
  g_zmq_context = NULL;
  g_zmq_poll_list = NULL;
  deinit_zmq();
  ck_assert_int_eq(g_zmq_context, NULL);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  /* Restore state */
  g_zmq_context = mock_ptr;
  g_zmq_poll_list = mock_poll;

  /* Check ctx is NULL and poll list isn't, mock_poll would be deallocated */
  g_zmq_context = NULL;
  deinit_zmq();
  ck_assert_int_eq(g_zmq_context, NULL);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  /* Restore state */
  g_zmq_context = mock_ptr;
  mock_poll = (zmq_pollitem_t *) malloc(sizeof(zmq_pollitem_t));
  g_zmq_poll_list = mock_poll;

  /* Check ctx isn't NULL and poll list is NULL */
  g_zmq_poll_list = NULL;
  deinit_zmq();
  ck_assert_int_eq(g_zmq_context, NULL);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  for (int i = 0; i < ZMQ_CONNECTION_COUNT; i++)
    {
    ck_assert_int_eq(g_svr_zconn[i].socket, NULL);
    }
  /* Restore state */
  g_zmq_context = mock_ptr;
  g_zmq_poll_list = mock_poll;
  for (int i = 0; i < ZMQ_CONNECTION_COUNT; i++)
    {
    g_svr_zconn[i].socket = mock_ptr;
    }

  /* Cleanup test data */
  free(mock_poll);
  free(mock_ptr);
  }
END_TEST

START_TEST(init_zmq_connection_test)
  {
  /* Testing function: init_zmq_connection()
   * Input parameters:
   *    enum zmq_connection_e id, (correct values: 0 - ZMQ_CONNECTION_COUNT-1)
   *          check: -1 if out of range
   *    int  socket_type, (ZMQ socket type)
   *          check: zmq_socket will fail if incorrect. Don't check
   * Output parameters:
   *    int (ret code)
   *          check: -1 if fail, 0 if success
   * Global variables:
   *    void *g_zmq_context
   *          check: fail if NULL,
   * Calling functions:
   *    log_err();
   *          check: assume never fail
   *    void *zmq_socket(void *, int) - return pointer or NULL
   *          check: -1 if NULL
   *    int add_zconnection(enum zmq_connection_e, void *, void *(*f)(void *), bool, bool)
   *          The same module, wouldn't be mocked.
   *          check: The function never failed with correct id.
   */
  /* Initialize globals */
  void *mock_ptr = malloc(sizeof(int));
  enum zmq_connection_e mockId = (enum zmq_connection_e)0;
  g_zmq_context = mock_ptr;
  g_svr_zconn[0].socket = NULL;
  g_zmq_socket_ret = mock_ptr;
  int rc = 0;

  /* Check wrong context */
  g_zmq_context = NULL;
  rc = init_zmq_connection(mockId, 0);
  ck_assert_int_eq(rc, -1);
  /* Restore state */
  g_zmq_context = mock_ptr;

  /* Check id */
  rc = init_zmq_connection((enum zmq_connection_e) -1, 0);
  ck_assert_int_eq(rc, -1);
  rc = init_zmq_connection((enum zmq_connection_e) ZMQ_CONNECTION_COUNT, 0);
  ck_assert_int_eq(rc, -1);
  rc = init_zmq_connection((enum zmq_connection_e) 0, 0);
  ck_assert_int_eq(rc, 0);
  rc = init_zmq_connection((enum zmq_connection_e) (ZMQ_CONNECTION_COUNT-1), 0);
  ck_assert_int_eq(rc, 0);

  /* Check can't create socket */
  g_zmq_socket_ret = NULL;
  rc = init_zmq_connection(mockId, 0);
  ck_assert_int_eq(rc, -1);
  /* Restore state */
  g_zmq_socket_ret = mock_ptr;

  /* Check good case */
  rc = init_zmq_connection(mockId, 0);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);
  /* Restore state */
  g_svr_zconn[0].socket = NULL;

  /* Cleanup test data */
  free(mock_ptr);
  }
END_TEST

START_TEST(close_zmq_connection_test)
  {
  /* Testing function: close_zmq_connection()
   * Input parameters:
   *    enum zmq_connection_e id, (correct values: 0 - ZMQ_CONNECTION_COUNT-1)
   *          check: -1 if out of range
   * Output parameters:
   *    int (ret code)
   *          check: -1 if fail, 0 if success
   * Global variables:
   *    void *g_zmq_context
   *          check: fail if NULL
   *    struct zconnection_s g_svr_zconn[ZMQ_CONNECTION_COUNT]
   *          check: for connection with the given id:
   *                 connected == false: success, do nothing
   *                 socket is NULL: fail, do nothing
   * Calling functions:
   *    log_err();
   *          check: assume never fail
   *    int zmq_getsockopt(void *, int, void *, size_t *) - return 0 if succes, -1 if fail
   *          check: -1 if failed
   *    int close_zmq_socket(void *) - return 0 if success, -1 if fail
   *          Local function, don't mock. Will fail if zmq_close() return -1.
   *          check: -1 if failed
   *    void *zmq_socket(void *, int) - return pointer or NULL
   *          check: -1 if NULL
   *    int add_zconnection(enum zmq_connection_e, void *, void *(*f)(void *), bool, bool)
   *          The same module, wouldn't be mocked.
   *          check: The function never failed with correct id. Check g_svr_zconn[id] socket is set
   *                 after this.
   */
  /* Initialize globals */
  void *mock_ptr = malloc(sizeof(int));
  void *mock_new_sock = malloc(sizeof(int));
  enum zmq_connection_e mockId = (enum zmq_connection_e)0;

  g_zmq_context = mock_ptr;
  g_svr_zconn[0].socket = mock_ptr;
  g_svr_zconn[0].connected = true;

  g_zmq_getsockopt_ret = 0;
  g_zmq_setsockopt_ret = 0;
  g_zmq_close_ret = 0;
  g_zmq_socket_ret = mock_new_sock;
  int rc = 0;

  /* Check wrong context */
  g_zmq_context = NULL;
  rc = close_zmq_connection(mockId);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);
  /* Restore state */
  g_zmq_context = mock_ptr;

  /* Check id */
  rc = close_zmq_connection((enum zmq_connection_e) -1);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);

  rc = close_zmq_connection((enum zmq_connection_e) ZMQ_CONNECTION_COUNT);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);

  rc = close_zmq_connection((enum zmq_connection_e) 0);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_new_sock);
  g_svr_zconn[0].socket = mock_ptr;

  g_svr_zconn[ZMQ_CONNECTION_COUNT-1].socket = mock_ptr;
  g_svr_zconn[ZMQ_CONNECTION_COUNT-1].connected = true;
  rc = close_zmq_connection((enum zmq_connection_e) (ZMQ_CONNECTION_COUNT-1));
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_svr_zconn[ZMQ_CONNECTION_COUNT-1].socket, mock_new_sock);
  g_svr_zconn[ZMQ_CONNECTION_COUNT-1].socket = mock_ptr;

  /* Check not connected */
  g_svr_zconn[0].connected = false;
  rc = close_zmq_connection(mockId);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);
  /* Restore state */
  g_svr_zconn[0].connected = true;

  /* Check wrong socket */
  g_svr_zconn[0].socket = NULL;
  rc = close_zmq_connection(mockId);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, NULL);
  /* Restore state */
  g_svr_zconn[0].socket = mock_ptr;

  /* Check can't get socket option */
  g_zmq_getsockopt_ret = -1;
  rc = close_zmq_connection(mockId);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);
  /* Restore state */
  g_zmq_getsockopt_ret = 0;

  /* Check close_zmq_socket failed */
  g_zmq_close_ret = -1;
  rc = close_zmq_connection(mockId);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);
  /* Restore state */
  g_zmq_close_ret = 0;
  
  /* Check can't create socket */
  g_zmq_socket_ret = NULL;
  rc = close_zmq_connection(mockId);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, NULL);
  /* Restore state */
  g_zmq_socket_ret = mock_new_sock;
  g_svr_zconn[0].socket = mock_ptr;

  /* Check good case */
  rc = close_zmq_connection(mockId);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_new_sock);
  ck_assert(!g_svr_zconn[0].connected);
  /* Restore state */
  g_svr_zconn[0].socket = mock_ptr;
  g_svr_zconn[0].connected = true;

  /* Cleanup test data */
  free(mock_ptr);
  free(mock_new_sock);
  }
END_TEST

START_TEST(add_zconnection_test)
  {
  /* Testing function: add_zconnection()
   * Input parameters:
   *    enum zmq_connection_e id, (correct values: 0 - ZMQ_CONNECTION_COUNT-1)
   *          check: -1 if out of range
   *    void *socket, (any pointer)
   *    void *(*f)(void *), (any pointer)
   *    bool should_poll, (any value)
   *    bool connected, (any value)
   *          check: check the values are actually assigned.
   * Output parameters:
   *    int (ret code)
   *          check: -1 if fail, 0 if success
   * Global variables:
   *    struct zconnection_s g_svr_zconn[ZMQ_CONNECTION_COUNT]
   *          check: all given parameters are assigned for the given connection
   * Calling functions:
   *    None.
   */
  /* Initialize globals */
  void *mock_ptr = malloc(sizeof(int));
  enum zmq_connection_e mockId = (enum zmq_connection_e)0;

  memset(g_svr_zconn, 0, sizeof(struct zconnection_s) * ZMQ_CONNECTION_COUNT);
  void *socket = mock_ptr;

  int rc = 0;

  /* Check id */
  rc = add_zconnection((enum zmq_connection_e)-1, socket, mockFunc, true, true);
  ck_assert_int_eq(rc, -1);
  rc = add_zconnection((enum zmq_connection_e)ZMQ_CONNECTION_COUNT, socket, mockFunc, true, true);
  ck_assert_int_eq(rc, -1);
  rc = add_zconnection((enum zmq_connection_e)0, socket, mockFunc, true, true);
  ck_assert_int_eq(rc, 0);
  rc = add_zconnection((enum zmq_connection_e)(ZMQ_CONNECTION_COUNT-1), socket, mockFunc, true, true);
  ck_assert_int_eq(rc, 0);
  /* Restore state */
  memset(g_svr_zconn, 0, sizeof(struct zconnection_s) * ZMQ_CONNECTION_COUNT);

  /* Check good case */
  rc = add_zconnection(mockId, socket, mockFunc, true, true);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, mock_ptr);
  ck_assert_int_eq(g_svr_zconn[0].func, mockFunc);
  ck_assert(g_svr_zconn[0].should_poll);
  ck_assert(g_svr_zconn[0].connected);
  /* Restore state */
  memset(g_svr_zconn, 0, sizeof(struct zconnection_s) * ZMQ_CONNECTION_COUNT);

  /* Cleanup test data */
  free(mock_ptr);
  }
END_TEST

START_TEST(process_status_request_test)
  {
  /* Testing function: process_status_request()
   * Input parameters:
   *    void *zsock, (zmq socket pointer)
   *          check: -1 if NULL
   *    int (*func)(const size_t, const char *), (any pointer)
   *          check: -1 if NULL
   *    bool wait, (passed to zmq_msg_recv())
   *          check: check correctly passed to zmq_msg_recv()
   * Output parameters:
   *    int (ret code)
   *          check: -1 if failed, 0 if success
   * Global variables:
   *    None
   * Calling functions:
   *    log_err();
   *          check: assume never fail
   *    int zmq_msg_init(zmq_msg_t *), -1 if fail, 0 if success
   *          check: -1 if failed
   *    int zmq_msg_recv(zmq_msg_t *, void *, int), -1 if fail, errno == EAGAIN means OK, but
   *                                                nothing to be read, number of bytes if success
   *          check: -1 if failed and errno isn't EAGAIN, 0 if failed, but errno is EAGAIN
   *    int zmq_getsockopt(void *, int, void *, size_t *), return 0 if succes, -1 if fail
   *          check: -1 if failed 
   *    size_t zmq_msg_size (zmq_msg_t *), return size in bytes. Never fail.
   *          check: Nothing. The result is always passed as is to func()
   *    void *zmq_msg_data (zmq_msg_t *msg), return a pointer to data. Declared as never fail.
   *          check: -1 if NULL
   *    int (*func)(const size_t, const char *), don't check the result. Failing of the function is
   *                                             not a problem of the function being tested.
   *          check: called expected times
   *    int zmq_msg_close(zmq_msg_t *), return -1 if fail, 0 if success
   *          check: -1 if failed
   */
  /* Initialize globals */
  void *mock_ptr = malloc(sizeof(int));
  char mock_data[] = "0123456789";
  size_t mock_data_size = 11; /* strlen("0123456789")+1 */

  void *socket = mock_ptr;
  g_zmq_msg_init_ret = 0;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_recv_ret = mock_data_size;
  g_zmq_msg_recv_errno = EAGAIN;
  g_zmq_msg_recv_count = 0;
  g_zmq_getsockopt_ret = 0;
  g_zmq_getsockopt_opt_rcvmore = false;
  g_zmq_msg_size_ret = mock_data_size;
  g_zmq_msg_data_ret = mock_data;
  g_zmq_msg_close_ret = 0;
  gs_data_processor_count = 0;

  int rc = 0;

  /* Check input */
  g_zmq_msg_init_count = 0;
  rc = process_status_request(NULL, mock_data_processor, false);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_msg_init_count, 0);

  rc = process_status_request(socket, NULL, false);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_msg_init_count, 0);

  /* Check zmq_msg_init */
  g_zmq_msg_init_ret = -1;
  g_zmq_msg_recv_count = 0;
  g_zmq_msg_close_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_msg_recv_count, 0);
  ck_assert_int_eq(g_zmq_msg_close_count, 0);
  g_zmq_msg_init_ret = 0;

  /* Check zmq_msg_recv */
  /*    1. Check there is no more data to be read */
  g_zmq_msg_recv_ret = -1;
  g_zmq_msg_recv_errno = EAGAIN;
  g_zmq_msg_size_count = 0;
  g_zmq_msg_data_count = 0;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_zmq_msg_size_count, 0);
  ck_assert_int_eq(g_zmq_msg_data_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  /* check the wait flag was passed correctly */
  ck_assert_int_eq(g_zmq_msg_recv_arg_flags, ZMQ_DONTWAIT);

  /*    2. Check error */
  g_zmq_msg_recv_ret = -1;
  g_zmq_msg_recv_errno = EFAULT;
  g_zmq_msg_size_count = 0;
  g_zmq_msg_data_count = 0;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  rc = process_status_request(socket, mock_data_processor, true);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_msg_size_count, 0);
  ck_assert_int_eq(g_zmq_msg_data_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  /* check the wait flag was passed correctly */
  ck_assert_int_eq(g_zmq_msg_recv_arg_flags, 0);
  g_zmq_msg_recv_ret = mock_data_size;

  /* Check zmq_getsockopt */
  g_zmq_getsockopt_ret = -1;
  gs_data_processor_count = 0;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(gs_data_processor_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  g_zmq_getsockopt_ret = 0;

  /* Check zmq_msg_data */
  g_zmq_msg_data_ret = NULL;
  gs_data_processor_count = 0;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(gs_data_processor_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  g_zmq_msg_data_ret = mock_data;

  /* Check zmq_msg_close */
  /*    1. Close inside loop after a good step */
  g_zmq_msg_close_ret = -1;
  gs_data_processor_count = 0;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);

  /*    2. Close after broken loop */
  g_zmq_msg_close_ret = -1;
  g_zmq_msg_recv_ret = -1;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  g_zmq_msg_close_ret = 0;
  g_zmq_msg_recv_ret = mock_data_size;

  /* Good cases */
  /*    1. Single-part message */
  g_zmq_getsockopt_opt_rcvmore = false;
  g_zmq_msg_recv_ret_map[2] = -1;
  g_zmq_msg_recv_count = 0;
  g_zmq_msg_recv_errno = EAGAIN;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  gs_data_processor_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  g_zmq_msg_recv_ret_map.clear();

  /*    2. 2 parts message */
  g_zmq_getsockopt_opt_rcvmore_map[1] = true;
  g_zmq_getsockopt_count = 0;
  g_zmq_msg_recv_ret_map[3] = -1;
  g_zmq_msg_recv_count = 0;
  g_zmq_msg_recv_errno = EAGAIN;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  gs_data_processor_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  g_zmq_getsockopt_opt_rcvmore_map.clear();
  g_zmq_msg_recv_ret_map.clear();

  /*    3. >2 parts message */
  g_zmq_getsockopt_opt_rcvmore_map[1] = true;
  g_zmq_getsockopt_opt_rcvmore_map[2] = true;
  g_zmq_getsockopt_count = 0;
  g_zmq_msg_recv_ret_map[3] = -1;
  g_zmq_msg_recv_count = 0;
  g_zmq_msg_recv_errno = EAGAIN;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_close_count = 0;
  gs_data_processor_count = 0;
  rc = process_status_request(socket, mock_data_processor, false);
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, 3);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  g_zmq_getsockopt_opt_rcvmore_map.clear();
  g_zmq_msg_recv_ret_map.clear();

  /* Cleanup test data */
  free(mock_ptr);
  }
END_TEST

Suite *zmq_common_suite(void)
  {
  Suite *s = suite_create("zmq_common_suite methods");
  TCase *tc_core = tcase_create("close_zmq_socket_test");
  tcase_add_test(tc_core, close_zmq_socket_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("init_zmq_test");
  tcase_add_test(tc_core, init_zmq_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("deinit_zmq_test");
  tcase_add_test(tc_core, deinit_zmq_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("init_zmq_connection_test");
  tcase_add_test(tc_core, init_zmq_connection_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("close_zmq_connection_test");
  tcase_add_test(tc_core, close_zmq_connection_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("add_zconnection_test");
  tcase_add_test(tc_core, add_zconnection_test);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("process_status_request_test");
  tcase_add_test(tc_core, process_status_request_test);
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
