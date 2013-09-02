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

static int                   gs_data_processor_count;
static zmq_pollitem_t       *gs_mock_poll;
static void                 *gs_mock_sock;
static void                 *gs_mock_new_sock;
static void                 *gs_mock_ctx;
static void                 *gs_mock_new_ctx;
static char                  gs_mock_data[255];
static size_t                gs_mock_data_size;
static void                 *gs_socket;
static int                   gs_rc;
static enum zmq_connection_e gs_mock_id;

void *mock_func(void *ptr)
  {
  return ptr;
  }

int mock_data_processor(const size_t sz, const char *data)
  {
  gs_data_processor_count++;
  return -1;
  }

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

void close_zmq_socket_tc_setup(void)
  {
  gs_mock_sock = malloc(sizeof(int));

  g_zmq_setsockopt_ret = 0;
  g_zmq_close_ret = 0;
  gs_socket = gs_mock_sock;

  gs_rc = 0;
  }

void close_zmq_socket_tc_teardown(void)
  {
  if (gs_mock_sock)
    {
    free(gs_mock_sock);
    }
  }

START_TEST(close_zmq_socket_test_input)
  {
  /* Check wrong input */
  gs_socket = NULL;
  gs_rc = close_zmq_socket(gs_socket);
  ck_assert_int_eq(gs_rc, -1);
  }
END_TEST

START_TEST(close_zmq_socket_test_zmq_setsockopt_fail)
  {
  /* Check return if zmq_setsockopt fail */
  g_zmq_setsockopt_ret = -1;
  gs_rc = close_zmq_socket(gs_socket);
  ck_assert_int_eq(gs_rc, 0);
  }
END_TEST

START_TEST(close_zmq_socket_test_zmq_close_fail)
  {
  /* Check return if zmq_close fail */
  g_zmq_close_ret = -1;
  gs_rc = close_zmq_socket(gs_socket);
  ck_assert_int_eq(gs_rc, -1);

  g_zmq_setsockopt_ret = -1;
  gs_rc = close_zmq_socket(gs_socket);
  ck_assert_int_eq(gs_rc, -1);
  }
END_TEST

START_TEST(close_zmq_socket_test_ok)
  {
  /* Check good case */
  gs_rc = close_zmq_socket(gs_socket);
  ck_assert_int_eq(gs_rc, 0);
  }
END_TEST

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

void init_zmq_tc_setup(void)
  {
  gs_mock_poll = (zmq_pollitem_t *) malloc(sizeof(zmq_pollitem_t));
  gs_mock_ctx = malloc(sizeof(int));
  gs_mock_new_ctx = malloc(sizeof(int));

  g_zmq_poll_list = gs_mock_poll;
  g_zmq_context = gs_mock_ctx;
  g_get_max_num_descriptors_ret = 2;
  g_zmq_ctx_new_ret = gs_mock_new_ctx;

  gs_rc = 0;
  }

void init_zmq_tc_teardown(void)
  {
  if (gs_mock_poll)
    {
    free(gs_mock_poll);
    }
  if (gs_mock_ctx)
    {
    free(gs_mock_ctx);
    }
  if (gs_mock_new_ctx)
    {
    free(gs_mock_new_ctx);
    }
  }

START_TEST(init_zmq_test_negative_max_num_descriptors)
  {
  /* Check get_max_num_descriptors() */
  g_get_max_num_descriptors_ret = -1;
  gs_rc = init_zmq();
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, NULL);
  }
END_TEST

START_TEST(init_zmq_test_zero_max_num_descriptors)
  {
  /* Check get_max_num_descriptors() */
  g_get_max_num_descriptors_ret = 0;
  gs_rc = init_zmq();
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, NULL);
  }
END_TEST

START_TEST(init_zmq_test_zmq_ctx_new_fail)
  {
  /* Check zmq_ctx_new */
  g_zmq_ctx_new_ret = NULL;
  gs_rc = init_zmq();
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, NULL);
  }
END_TEST

START_TEST(init_zmq_test_ok)
  {
  /* Check good case */
  gs_rc = init_zmq();
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_ne(g_zmq_poll_list, NULL);
  ck_assert_int_eq(g_zmq_context, gs_mock_new_ctx);
  for (int i = 0; i < g_get_max_num_descriptors_ret + ZMQ_CONNECTION_COUNT; i++)
    {
    ck_assert_int_eq(g_zmq_poll_list[i].events, ZMQ_POLLIN);
    }
  }
END_TEST

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

void deinit_zmq_tc_setup(void)
  {
  gs_mock_poll = (zmq_pollitem_t *) malloc(sizeof(zmq_pollitem_t));
  gs_mock_ctx = malloc(sizeof(int));
  gs_mock_sock = malloc(sizeof(int));

  g_zmq_context = gs_mock_ctx;
  g_zmq_poll_list = gs_mock_poll;

  for (int i = 0; i < ZMQ_CONNECTION_COUNT; i++)
    {
    g_svr_zconn[i].socket = gs_mock_sock;
    }
  g_zmq_ctx_destroy_ret = -1;
  }

void deinit_zmq_tc_teardown(void)
  {
  if (gs_mock_ctx)
    {
    free(gs_mock_ctx);
    }
  if (gs_mock_sock)
    {
    free(gs_mock_sock);
    }
  if (gs_mock_poll)
    {
    free(gs_mock_poll);
    }
  }

START_TEST(deinit_zmq_test_wrong_zmq_context_and_zmq_poll_list)
  {
  /* Check ctx and poll list are NULL */
  g_zmq_context = NULL;
  g_zmq_poll_list = NULL;
  deinit_zmq();
  ck_assert_int_eq(g_zmq_context, NULL);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  }
END_TEST

START_TEST(deinit_zmq_test_wrong_zmq_context)
  {
  /* Check ctx is NULL and poll list isn't, mock_poll would be deallocated */
  g_zmq_context = NULL;
  deinit_zmq();
  ck_assert_int_eq(g_zmq_context, NULL);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  gs_mock_poll = NULL; // Poll is freed by deinit_zmq()
  }
END_TEST

START_TEST(deinit_zmq_test_wrong_poll_list)
  {
  /* Check ctx isn't NULL and poll list is NULL */
  g_zmq_poll_list = NULL;
  g_svr_zconn[0].socket = NULL; // Also check the case when a socket is NULL
  deinit_zmq();
  ck_assert_int_eq(g_zmq_context, NULL);
  ck_assert_int_eq(g_zmq_poll_list, NULL);
  for (int i = 0; i < ZMQ_CONNECTION_COUNT; i++)
    {
    ck_assert_int_eq(g_svr_zconn[i].socket, NULL);
    }
  }
END_TEST

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

void init_zmq_connection_tc_setup(void)
  {
  gs_mock_ctx = malloc(sizeof(int));
  gs_mock_new_sock = malloc(sizeof(int));
  gs_mock_id = (enum zmq_connection_e)0;

  g_zmq_context = gs_mock_ctx;
  g_svr_zconn[0].socket = NULL;

  g_zmq_socket_ret = gs_mock_new_sock;

  gs_rc = 0;
  }

void init_zmq_connection_tc_teardown(void)
  {
  if (gs_mock_ctx)
    {
    free(gs_mock_ctx);
    }
  if (gs_mock_new_sock)
    {
    free(gs_mock_new_sock);
    }
  }

START_TEST(init_zmq_connection_test_wrong_input)
  {
  /* Check id */
  gs_rc = init_zmq_connection((enum zmq_connection_e) -1, 0);
  ck_assert_int_eq(gs_rc, -1);
  gs_rc = init_zmq_connection((enum zmq_connection_e) ZMQ_CONNECTION_COUNT, 0);
  ck_assert_int_eq(gs_rc, -1);
  gs_rc = init_zmq_connection((enum zmq_connection_e) 0, 0);
  ck_assert_int_eq(gs_rc, 0);
  gs_rc = init_zmq_connection((enum zmq_connection_e) (ZMQ_CONNECTION_COUNT-1), 0);
  ck_assert_int_eq(gs_rc, 0);
  }
END_TEST

START_TEST(init_zmq_connection_test_wrong_zmq_context)
  {
  /* Check wrong context */
  g_zmq_context = NULL;
  gs_rc = init_zmq_connection(gs_mock_id, 0);
  ck_assert_int_eq(gs_rc, -1);
  }
END_TEST

START_TEST(init_zmq_connection_test_zmq_socket_fail)
  {
  /* Check can't create socket */
  g_zmq_socket_ret = NULL;
  gs_rc = init_zmq_connection(gs_mock_id, 0);
  ck_assert_int_eq(gs_rc, -1);
  }
END_TEST

START_TEST(init_zmq_connection_test_ok)
  {
  /* Check good case */
  gs_rc = init_zmq_connection(gs_mock_id, 0);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_new_sock);
  }
END_TEST

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

void close_zmq_connection_tc_setup(void)
  {
  gs_mock_ctx = malloc(sizeof(int));
  gs_mock_sock = malloc(sizeof(int));
  gs_mock_new_sock = malloc(sizeof(int));
  gs_mock_id = (enum zmq_connection_e)0;

  g_zmq_context = gs_mock_ctx;
  g_svr_zconn[0].socket = gs_mock_sock;
  g_svr_zconn[0].connected = true;

  g_zmq_getsockopt_ret = 0;
  g_zmq_setsockopt_ret = 0;
  g_zmq_close_ret = 0;
  g_zmq_socket_ret = gs_mock_new_sock;

  gs_rc = 0;
  }

void close_zmq_connection_tc_teardown(void)
  {
  if (gs_mock_ctx)
    {
    free(gs_mock_ctx);
    }
  if (gs_mock_sock)
    {
    free(gs_mock_sock);
    }
  if (gs_mock_new_sock)
    {
    free(gs_mock_new_sock);
    }
  }

START_TEST(close_zmq_connection_test_input)
  {
  /* Check id */
  gs_rc = close_zmq_connection((enum zmq_connection_e) -1);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_sock);

  gs_rc = close_zmq_connection((enum zmq_connection_e) ZMQ_CONNECTION_COUNT);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_sock);

  gs_rc = close_zmq_connection((enum zmq_connection_e) 0);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_new_sock);
  g_svr_zconn[0].socket = gs_mock_sock;

  g_svr_zconn[ZMQ_CONNECTION_COUNT-1].socket = gs_mock_sock;
  g_svr_zconn[ZMQ_CONNECTION_COUNT-1].connected = true;
  gs_rc = close_zmq_connection((enum zmq_connection_e) (ZMQ_CONNECTION_COUNT-1));
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(g_svr_zconn[ZMQ_CONNECTION_COUNT-1].socket, gs_mock_new_sock);
  }
END_TEST

START_TEST(close_zmq_connection_test_wrong_zmq_context)
  {
  /* Check wrong context */
  g_zmq_context = NULL;
  gs_rc = close_zmq_connection(gs_mock_id);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_sock);
  }
END_TEST

START_TEST(close_zmq_connection_test_not_connected)
  {
  /* Check not connected */
  g_svr_zconn[0].connected = false;
  gs_rc = close_zmq_connection(gs_mock_id);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_sock);
  }
END_TEST

START_TEST(close_zmq_connection_test_null_socket)
  {
  /* Check wrong socket */
  g_svr_zconn[0].socket = NULL;
  gs_rc = close_zmq_connection(gs_mock_id);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, NULL);
  }
END_TEST

START_TEST(close_zmq_connection_test_zmq_getsockopt_fail)
  {
  /* Check can't get socket option */
  g_zmq_getsockopt_ret = -1;
  gs_rc = close_zmq_connection(gs_mock_id);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_sock);
  }
END_TEST

START_TEST(close_zmq_connection_test_close_zmq_socket_fail)
  {
  /* Check close_zmq_socket failed */
  g_zmq_close_ret = -1;
  gs_rc = close_zmq_connection(gs_mock_id);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_sock);
  }
END_TEST

START_TEST(close_zmq_connection_test_zmq_socket_fail)
  {
  /* Check can't create socket */
  g_zmq_socket_ret = NULL;
  gs_rc = close_zmq_connection(gs_mock_id);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_svr_zconn[0].socket, NULL);
  }
END_TEST

START_TEST(close_zmq_connection_test_ok)
  {
  /* Check good case */
  gs_rc = close_zmq_connection(gs_mock_id);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_new_sock);
  ck_assert(!g_svr_zconn[0].connected);
  }
END_TEST

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

void add_zconnection_tc_setup(void)
  {
  gs_mock_sock = malloc(sizeof(int));
  gs_mock_id = (enum zmq_connection_e)0;
  memset(g_svr_zconn, 0, sizeof(struct zconnection_s) * ZMQ_CONNECTION_COUNT);

  gs_socket = gs_mock_sock;

  gs_rc = 0;
  }

void add_zconnection_tc_teardown(void)
  {
  if (gs_mock_sock)
    {
    free(gs_mock_sock);
    }
  }

START_TEST(add_zconnection_test_input)
  {
  /* Check id */
  gs_rc = add_zconnection((enum zmq_connection_e)-1, gs_socket, mock_func, true, true);
  ck_assert_int_eq(gs_rc, -1);
  gs_rc = add_zconnection((enum zmq_connection_e)ZMQ_CONNECTION_COUNT, gs_socket, mock_func, true, true);
  ck_assert_int_eq(gs_rc, -1);
  gs_rc = add_zconnection((enum zmq_connection_e)0, gs_socket, mock_func, true, true);
  ck_assert_int_eq(gs_rc, 0);
  gs_rc = add_zconnection((enum zmq_connection_e)(ZMQ_CONNECTION_COUNT-1), gs_socket, mock_func, true, true);
  ck_assert_int_eq(gs_rc, 0);
  }
END_TEST

START_TEST(add_zconnection_test_ok)
  {
  /* Check good case */
  gs_rc = add_zconnection(gs_mock_id, gs_socket, mock_func, true, true);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(g_svr_zconn[0].socket, gs_mock_sock);
  ck_assert_int_eq(g_svr_zconn[0].func, mock_func);
  ck_assert(g_svr_zconn[0].should_poll);
  ck_assert(g_svr_zconn[0].connected);
  }
END_TEST

/*
 * Testing function: process_status_request()
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

void process_status_request_tc_setup(void)
  {
  gs_mock_sock = malloc(sizeof(int));
  strcpy(gs_mock_data, "0123456789");
  gs_mock_data_size = 11; /* strlen("0123456789")+1 */

  g_zmq_msg_recv_ret_map.clear();
  g_zmq_getsockopt_opt_rcvmore_map.clear();

  gs_socket = gs_mock_sock;
  g_zmq_getsockopt_ret = 0;
  g_zmq_getsockopt_count = 0;
  g_zmq_getsockopt_opt_rcvmore = false;
  g_zmq_msg_close_count = 0;
  g_zmq_msg_close_ret = 0;
  g_zmq_msg_data_count = 0;
  g_zmq_msg_data_ret = gs_mock_data;
  g_zmq_msg_init_count = 0;
  g_zmq_msg_init_ret = 0;
  g_zmq_msg_recv_count = 0;
  g_zmq_msg_recv_errno = EAGAIN;
  g_zmq_msg_recv_ret = gs_mock_data_size;
  g_zmq_msg_size_count = 0;
  g_zmq_msg_size_ret = gs_mock_data_size;
  gs_data_processor_count = 0;

  gs_rc = 0;
  }

void process_status_request_tc_teardown(void)
  {
  if (gs_mock_sock)
    {
    free(gs_mock_sock);
    }
  }

START_TEST(process_status_request_test_input)
  {
  /* Check input */
  gs_rc = process_status_request(NULL, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_msg_init_count, 0);

  gs_rc = process_status_request(gs_socket, NULL, false);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_msg_init_count, 0);
  }
END_TEST

START_TEST(process_status_request_test_zmq_msg_init_fail)
  {
  g_zmq_msg_init_ret = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_msg_recv_count, 0);
  ck_assert_int_eq(g_zmq_msg_close_count, 0);
  }
END_TEST

START_TEST(process_status_request_test_zmq_msg_recv_fail_no_data)
  {
  /* Check zmq_msg_recv */
  /* 1. Check there is no more data to be read */
  g_zmq_msg_recv_ret = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(g_zmq_msg_size_count, 0);
  ck_assert_int_eq(g_zmq_msg_data_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  /* check the wait flag was passed correctly */
  ck_assert_int_eq(g_zmq_msg_recv_arg_flags, ZMQ_DONTWAIT);
  }
END_TEST

START_TEST(process_status_request_test_zmq_msg_recv_fail)
  {
  /* Check zmq_msg_recv */
  /* 2. Check error */
  g_zmq_msg_recv_ret = -1;
  g_zmq_msg_recv_errno = EFAULT;
  gs_rc = process_status_request(gs_socket, mock_data_processor, true);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_msg_size_count, 0);
  ck_assert_int_eq(g_zmq_msg_data_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  /* check the wait flag was passed correctly */
  ck_assert_int_eq(g_zmq_msg_recv_arg_flags, 0);
  }
END_TEST

START_TEST(process_status_request_test_zmq_getsockopt_fail)
  {
  /* Check zmq_getsockopt */
  g_zmq_getsockopt_ret = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(gs_data_processor_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  }
END_TEST

START_TEST(process_status_request_test_zmq_msg_data_fail)
  {
  /* Check zmq_msg_data */
  g_zmq_msg_data_ret = NULL;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(gs_data_processor_count, 0);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  }
END_TEST

START_TEST(process_status_request_test_zmq_msg_close_fail_in_loop)
  {
  /* Check zmq_msg_close */
  /* 1. Close inside loop after a good step */
  g_zmq_msg_close_ret = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  }
END_TEST

START_TEST(process_status_request_test_zmq_msg_close_fail_after_loop)
  {
  /* Check zmq_msg_close */
  /* 2. Close after broken loop */
  g_zmq_msg_close_ret = -1;
  g_zmq_msg_recv_ret = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, -1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  }
END_TEST

START_TEST(process_status_request_test_single_part)
  {
  /* Good cases */
  /* 1. Single-part message */
  g_zmq_msg_recv_ret_map[2] = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  }
END_TEST

START_TEST(process_status_request_test_double_part)
  {
  /* Good cases */
  /* 2. 2 parts message */
  g_zmq_getsockopt_opt_rcvmore_map[1] = true;
  g_zmq_msg_recv_ret_map[3] = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  }
END_TEST

START_TEST(process_status_request_test_triple_part)
  {
  /* Good cases */
  /* 3. >2 parts message */
  g_zmq_getsockopt_opt_rcvmore_map[1] = true;
  g_zmq_getsockopt_opt_rcvmore_map[2] = true;
  g_zmq_msg_recv_ret_map[3] = -1;
  gs_rc = process_status_request(gs_socket, mock_data_processor, false);
  ck_assert_int_eq(gs_rc, 0);
  ck_assert_int_eq(gs_data_processor_count, 1);
  ck_assert_int_eq(g_zmq_msg_init_count, 3);
  ck_assert_int_eq(g_zmq_msg_init_count, g_zmq_msg_close_count);
  }
END_TEST

Suite *zmq_common_suite(void)
  {
  Suite *s = suite_create("zmq_common_suite methods");
  TCase *tc_core = tcase_create("close_zmq_socket_test");
  tcase_add_checked_fixture(tc_core, close_zmq_socket_tc_setup, close_zmq_socket_tc_teardown);
  tcase_add_test(tc_core, close_zmq_socket_test_input);
  tcase_add_test(tc_core, close_zmq_socket_test_zmq_setsockopt_fail);
  tcase_add_test(tc_core, close_zmq_socket_test_zmq_close_fail);
  tcase_add_test(tc_core, close_zmq_socket_test_ok);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("init_zmq_test");
  tcase_add_checked_fixture(tc_core, init_zmq_tc_setup, init_zmq_tc_teardown);
  tcase_add_test(tc_core, init_zmq_test_negative_max_num_descriptors);
  tcase_add_test(tc_core, init_zmq_test_zero_max_num_descriptors);
  tcase_add_test(tc_core, init_zmq_test_zmq_ctx_new_fail);
  tcase_add_test(tc_core, init_zmq_test_ok);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("deinit_zmq_test");
  tcase_add_checked_fixture(tc_core, deinit_zmq_tc_setup, deinit_zmq_tc_teardown);
  tcase_add_test(tc_core, deinit_zmq_test_wrong_zmq_context_and_zmq_poll_list);
  tcase_add_test(tc_core, deinit_zmq_test_wrong_zmq_context);
  tcase_add_test(tc_core, deinit_zmq_test_wrong_poll_list);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("init_zmq_connection_test");
  tcase_add_checked_fixture(tc_core, init_zmq_connection_tc_setup, init_zmq_connection_tc_teardown);
  tcase_add_test(tc_core, init_zmq_connection_test_wrong_input);
  tcase_add_test(tc_core, init_zmq_connection_test_wrong_zmq_context);
  tcase_add_test(tc_core, init_zmq_connection_test_zmq_socket_fail);
  tcase_add_test(tc_core, init_zmq_connection_test_ok);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("close_zmq_connection_test");
  tcase_add_checked_fixture(tc_core, close_zmq_connection_tc_setup,
      close_zmq_connection_tc_teardown);
  tcase_add_test(tc_core, close_zmq_connection_test_input);
  tcase_add_test(tc_core, close_zmq_connection_test_wrong_zmq_context);
  tcase_add_test(tc_core, close_zmq_connection_test_not_connected);
  tcase_add_test(tc_core, close_zmq_connection_test_null_socket);
  tcase_add_test(tc_core, close_zmq_connection_test_zmq_getsockopt_fail);
  tcase_add_test(tc_core, close_zmq_connection_test_close_zmq_socket_fail);
  tcase_add_test(tc_core, close_zmq_connection_test_zmq_socket_fail);
  tcase_add_test(tc_core, close_zmq_connection_test_ok);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("add_zconnection_test");
  tcase_add_checked_fixture(tc_core, add_zconnection_tc_setup, add_zconnection_tc_teardown);
  tcase_add_test(tc_core, add_zconnection_test_input);
  tcase_add_test(tc_core, add_zconnection_test_ok);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("process_status_request_test");
  tcase_add_checked_fixture(tc_core, process_status_request_tc_setup,
      process_status_request_tc_teardown);
  tcase_add_test(tc_core, process_status_request_test_input);
  tcase_add_test(tc_core, process_status_request_test_zmq_msg_init_fail);
  tcase_add_test(tc_core, process_status_request_test_zmq_msg_recv_fail_no_data);
  tcase_add_test(tc_core, process_status_request_test_zmq_msg_recv_fail);
  tcase_add_test(tc_core, process_status_request_test_zmq_getsockopt_fail);
  tcase_add_test(tc_core, process_status_request_test_zmq_msg_data_fail);
  tcase_add_test(tc_core, process_status_request_test_zmq_msg_close_fail_in_loop);
  tcase_add_test(tc_core, process_status_request_test_zmq_msg_close_fail_after_loop);
  tcase_add_test(tc_core, process_status_request_test_single_part);
  tcase_add_test(tc_core, process_status_request_test_double_part);
  tcase_add_test(tc_core, process_status_request_test_triple_part);
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
