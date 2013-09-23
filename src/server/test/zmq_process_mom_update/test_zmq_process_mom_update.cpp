#include <stdlib.h>
#include <map>
#include <jsoncpp/json/json.h>

#include "zmq_process_mom_update.h"
#include "test_zmq_process_mom_update.h"
#include "pbs_nodes.h"
#include "dis.h"

extern int  allow_any_mom;
extern int  g_decode_arst_ret;
extern int  g_decode_arst_ret_count;
extern std::map<int, int> g_decode_arst_ret_map;
extern int  g_free_arst_ret_count;
extern int  g_node_gpustatus_list_ret;
extern int  g_node_gpustatus_list_ret_count;
extern int  g_update_nodes_file_ret;
extern int  g_update_nodes_file_ret_count;
extern int  g_gpu_entry_by_id_ret;
extern int  g_gpu_has_job_ret;
extern int  g_node_micstatus_list_ret;
extern int  g_node_micstatus_list_ret_count;
extern int  g_clear_nvidia_gpus_ret_count;
extern int  g_enqueue_threadpool_request_ret_count;
extern long g_get_svr_attr_l_ret_mom_down_on_error;
extern long g_get_svr_attr_l_ret_mom_job_sync;
extern int  g_handle_auto_np_val_ret_count;
extern int  g_process_state_str_val_ret_count;
extern int  g_process_uname_str_ret_count;
extern int  g_remove_hello_ret_count;
extern int  g_update_job_data_ret_count;
extern int  g_update_node_state_ret_count;
extern struct pbsnode * g_find_nodebyname_ret;

static struct pbsnode *gs_mock_node;
static char gs_mock_node_name[] = "test_node";

struct pbsnode *init_node()
  {
  struct pbsnode *np = (struct pbsnode *) calloc(1, sizeof(struct pbsnode));
  np->nd_name = gs_mock_node_name;
  np->nd_gpusn = (gpusubn *)calloc(2, sizeof(struct gpusubn));

  return np;
  }

void deinit_node(struct pbsnode *np)
  {
  free(np->nd_gpusn);
  free(np);
  }

namespace TrqZStatus {

class TestHelper
  {
  private:

    MomUpdate &momUpdate;

  public:

    TestHelper(MomUpdate &momUpdate) : momUpdate(momUpdate) {}

    int pbsReadJsonGpuStatus(Json::Value &gpusStatus)
      {
      return momUpdate.pbsReadJsonGpuStatus(gpusStatus);
      }

    int pbsReadJsonMicStatus(Json::Value &micsStatus)
      {
      return momUpdate.pbsReadJsonMicStatus(micsStatus);
      }

    struct pbsnode *getCurrentNode()
      {
      return momUpdate.m_current_node;
      }

    void setCurrentNode(struct pbsnode *node)
      {
      momUpdate.m_current_node = node;
      }
  };

} /* END namespace TrqZStatus */

using namespace TrqZStatus;

static MomUpdate *gs_mom_update;
static TestHelper *gs_test_helper;


/* Testing function: MomUpdate::pbsReadJsonGpuStatus()
 * Input parameters:
 *    Json::Value &gpus_status - jsoncpp value with gpu statuses
 *          check: should be array value, fail with DIS_NOCOMMIT if not
 * Using members:
 *    struct pbsnode *m_current_node - the node to be updated
 *          check: Nothing special
 * Output parameters:
 *    int, (ret code)
 *          check: DIS_NOCOMMIT if fail to get statuses,
 *                 DIS_SUCCESS if read successfully or if gpuidx couldn't be retrieved.
 * Calling functions:
 *    int decode_arst() - return PBSE_NONE if success
 *          check: called with NULL for initialization. called after parsed each gpu
 *    void free_arst(pbs_attribute *) - free attribute data
 *          check: called once for each new attribute.
 *    int gpu_entry_by_id() - return -1 if fail
 *          check: return DIS_SUCCESS, no changes in the node if failed
 *    int update_nodes_file(struct pbsnode *)
 *          check: called if gpu count is changed
 *    int node_gpustatus_list(pbs_attribute *, void *, int)
 *          check: called after all parsed
 */

void pbs_read_json_gpu_status_tc_setup(void)
  {
  gs_mock_node = init_node();
  gs_mom_update = new MomUpdate();
  gs_test_helper = new TestHelper(*gs_mom_update);
  gs_test_helper->setCurrentNode(gs_mock_node);

  g_decode_arst_ret = 0;
  g_decode_arst_ret_count = 0;
  g_decode_arst_ret_map.clear();
  g_free_arst_ret_count = 0;
  g_node_gpustatus_list_ret = 0;
  g_node_gpustatus_list_ret_count = 0;
  g_update_nodes_file_ret_count = 0;
  g_gpu_entry_by_id_ret = 0;
  }

void pbs_read_json_gpu_status_tc_teardown(void)
  {
  deinit_node(gs_mock_node);
  g_decode_arst_ret_map.clear();
  delete(gs_test_helper);
  delete(gs_mom_update);
  }

START_TEST(pbs_read_json_gpu_status_test_null_value)
  {
  int rc;
  /* Check null value passed */
  /* No steps should be performed */
  Json::Value nullValue;
  rc = gs_test_helper->pbsReadJsonGpuStatus(nullValue);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_non_object_value)
  {
  int rc;
  /* Check non-object value passed */
  Json::Value arrayValue(Json::arrayValue);
  rc = gs_test_helper->pbsReadJsonGpuStatus(arrayValue);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_non_array_value)
  {
  int rc;
  /* Check non-array value passed */
  Json::Value status;
  Json::Value gpus(Json::objectValue);
  status["gpus"] = gpus;

  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_attribute_init_fail)
  {
  int rc;
  /* Check decode_arst fail */
  Json::Value status(Json::objectValue);
  Json::Value gpus(Json::arrayValue);
  status["gpus"] = gpus;

  g_decode_arst_ret = -1;
  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_attribute_decode_fail)
  {
  int rc;

  Json::Value status(Json::objectValue);
  status["timestamp"] = "1234567890";
  status["driver_ver"] = "1234.00033";
  Json::Value gpu1;
  Json::Value gpu2;

  gpu1["gpuid"] = "gpu1";
  gpu1["gpu_mode"] = "Normal";
  gpu2["gpuid"] = "gpu2";
  gpu2["gpu_mode"] = "Exclusive";

  status["gpus"].append(gpu1);
  status["gpus"].append(gpu2);

  g_decode_arst_ret_map[4] = -1;
  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 4); // Init, timestamp, driver_verstion, save gpu1
  ck_assert_int_eq(g_free_arst_ret_count, 1);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 0); // node was updated
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_empty_array)
  {
  int rc;
  /* Check value is empty array */
  Json::Value status(Json::objectValue);
  Json::Value gpus(Json::arrayValue);
  status["gpus"] = gpus;

  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 1); // init only
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_no_gpuid)
  {
  int rc;
  /* Check one of gpus have no gpuid field */
  Json::Value status(Json::objectValue);
  Json::Value gpu1;
  gpu1["gpuid"] = "gpu1";
  gpu1["testKey"] = "testValue";
  Json::Value gpu2;
  gpu2["testKey"] = "testValue";
  status["gpus"].append(gpu1);
  status["gpus"].append(gpu2);

  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_free_arst_ret_count, 1);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 0); // node wasn't touched
  ck_assert(gs_mock_node->nd_gpus_real);
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_no_gpuidx_found)
  {
  int rc;

  Json::Value status(Json::objectValue);
  Json::Value gpu1;
  gpu1["gpuid"] = "gpu1";
  gpu1["testKey"] = "testValue";
  status["gpus"].append(gpu1);

  g_gpu_entry_by_id_ret = -1;
  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 1); // init only
  ck_assert_int_eq(g_free_arst_ret_count, 1);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 0); // node wasn't touched
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_correct_statuses)
  {
  int rc;

  Json::Value status;
  Json::Value gpu1;
  Json::Value gpu2;

  gpu1["gpuid"] = "gpu1";
  gpu1["gpu_mode"] = "Normal";
  gpu2["gpuid"] = "gpu2";
  gpu2["gpu_mode"] = "Exclusive";

  status["timestamp"] = "1234567890";
  status["driver_ver"] = "1234.00033";
  status["gpus"].append(gpu1);
  status["gpus"].append(gpu2);

  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 5); // init, 2 values, 2 gpus
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 1);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 1); // node was updated
  g_decode_arst_ret_count = 0;
  g_free_arst_ret_count = 0;
  g_update_nodes_file_ret_count = 0;
  g_node_gpustatus_list_ret_count = 0;

  status["gpus"][0]["gpuid"] = "gpu1";
  status["gpus"][1]["gpuid"] = "gpu2";
  status["gpus"][0]["gpu_mode"] = "Default";
  status["gpus"][1]["gpu_mode"] = "Exclusive_Thread";

  g_gpu_has_job_ret = 1;
  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 5); // init, 2 values, 2 gpus
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 1); // node was updated
  g_decode_arst_ret_count = 0;
  g_free_arst_ret_count = 0;
  g_update_nodes_file_ret_count = 0;
  g_node_gpustatus_list_ret_count = 0;

  status["gpus"][0]["gpuid"] = "gpu1";
  status["gpus"][1]["gpuid"] = "gpu2";
  status["gpus"][0]["gpu_mode"] = "Exclusive_Process";
  status["gpus"][1]["gpu_mode"] = "Prohibited";

  g_gpu_has_job_ret = 0;
  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 5); // init, 2 values, 2 gpus
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 1); // node was updated
  g_decode_arst_ret_count = 0;
  g_free_arst_ret_count = 0;
  g_update_nodes_file_ret_count = 0;
  g_node_gpustatus_list_ret_count = 0;

  status["gpus"][0]["gpuid"] = "gpu1";
  status["gpus"][1]["gpuid"] = "gpu2";
  status["gpus"][0]["gpu_mode"] = "Exclusive_Process";
  status["gpus"][1]["gpu_mode"] = "Somewhere_In_The_Clouds";

  g_gpu_has_job_ret = 1;
  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 5); // init, 2 values, 2 gpus
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 1); // node was updated
  }
END_TEST

START_TEST(pbs_read_json_gpu_status_test_bad_status_values)
  {
  int rc;
  Json::Value gpu1;
  Json::Value gpu2;
  Json::Value status;

  gpu1["gpuid"] = "gpu1";
  gpu1["gpu_mode"] = 6174;

  Json::Value v1;
  Json::Value v2;

  gpu2["gpuid"] = "gpu2";
  gpu2["gpu_mode"] = Json::Value();

  status["timestamp"] = 6174;
  status["driver_ver"] = 6174;
  status["gpus"].append(gpu1);
  status["gpus"].append(gpu2);

  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 4); // init, driver_ver, 2 gpus
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 1);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 1); // node was updated

  g_decode_arst_ret_count = 0;
  g_free_arst_ret_count = 0;
  g_update_nodes_file_ret_count = 0;
  g_node_gpustatus_list_ret_count = 0;

  status["gpus"][0]["gpuid"] = "gpu1";
  status["gpus"][1]["gpuid"] = "gpu2";

  v1[0] = "Spock";
  v1[1] = "Kirk";
  status["timestamp"] = v1;
  v2["Father"] = "Homer";
  v2["Mother"] = "Marge";
  status["driver_ver"] = v2;

  rc = gs_test_helper->pbsReadJsonGpuStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 3); // init, 2 gpus
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  ck_assert_int_eq(g_node_gpustatus_list_ret_count, 1); // node was updated
  }
END_TEST

/* Testing function: MomUpdate::pbsReadJsonMicStatus()
 * Input parameters:
 *    Json::Value &mics_status - jsoncpp value with mic statuses
 *          check: should be array value, fail with DIS_NOCOMMIT if not
 * Using memgers:
 *    struct pbsnode *m_current_node - the node to be updated
 *          check: Nothing special
 * Output parameters:
 *    int, (ret code)
 *          check: DIS_NOCOMMIT if fail to get statuses,
 * Calling functions:
 *    int decode_arst() - return PBSE_NONE if success
 *          check: called with NULL for initialization. called after parsed each mic
 *    void free_arst(pbs_attribute *) - free attribute data
 *          check: called once for each new attribute.
 *    int node_micstatus_list(pbs_attribute *, void *, int)
 *          check: called after all parsed
 */

void pbs_read_json_mic_status_tc_setup(void)
  {
  gs_mock_node = init_node();
  gs_mom_update = new MomUpdate();
  gs_test_helper = new TestHelper(*gs_mom_update);
  gs_test_helper->setCurrentNode(gs_mock_node);

  g_decode_arst_ret = 0;
  g_decode_arst_ret_count = 0;
  g_decode_arst_ret_map.clear();
  g_free_arst_ret_count = 0;
  g_node_micstatus_list_ret = 0;
  g_node_micstatus_list_ret_count = 0;
  }

void pbs_read_json_mic_status_tc_teardown(void)
  {
  deinit_node(gs_mock_node);
  g_decode_arst_ret_map.clear();
  delete(gs_test_helper);
  delete(gs_mom_update);
  }

START_TEST(pbs_read_json_mic_status_test_null_value)
  {
  int rc;
  /* Check null value passed */
  /* No steps should be performed */
  Json::Value nullValue;
  rc = gs_test_helper->pbsReadJsonMicStatus(nullValue);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_mic_status_test_non_object_value)
  {
  int rc;
  /* Check non-object value passed */
  Json::Value arrayValue(Json::arrayValue);
  rc = gs_test_helper->pbsReadJsonMicStatus(arrayValue);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_mic_status_test_non_array_value)
  {
  int rc;
  /* Check non-array value passed as mics */
  Json::Value status;
  Json::Value mics(Json::objectValue);
  status["mics"] = mics;

  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_mic_status_test_attribute_init_fail)
  {
  int rc;
  /* Check decode_arst fail */
  Json::Value status(Json::objectValue);
  Json::Value mics(Json::arrayValue);
  status["mics"] = mics;

  g_decode_arst_ret = -1;
  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_mic_status_test_attribute_decode_fail)
  {
  int rc;

  Json::Value status;
  Json::Value mic1;
  Json::Value mic2;

  mic1["micid"] = "mic1";
  mic1["key1"] = "1234567890";
  mic1["key2"] = "1234.00033";
  mic2["micid"] = "mic2";
  mic2["key1"] = "1234567890";
  mic2["key2"] = "1234.00033";

  status["mics"].append(mic1);
  status["mics"].append(mic2);

  g_decode_arst_ret_map[2] = -1;
  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_free_arst_ret_count, 1);
  ck_assert_int_eq(g_node_micstatus_list_ret_count, 1); // node was updated
  }
END_TEST

START_TEST(pbs_read_json_mic_status_test_empty_array)
  {
  int rc;
  /* Check value is empty array */
  Json::Value status(Json::objectValue);
  Json::Value mics(Json::arrayValue);
  status["mics"] = mics;

  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 1); // init only
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_update_nodes_file_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_mic_status_test_no_micid)
  {
  int rc;
  /* Check one of mics have no micid field */
  Json::Value status;
  Json::Value mic1;
  mic1["micid"] = "mic1";
  mic1["testKey"] = "testValue";
  Json::Value mic2;
  mic2["testKey"] = "testValue";
  status["mics"].append(mic1);
  status["mics"].append(mic2);

  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, DIS_NOCOMMIT);
  ck_assert_int_eq(g_free_arst_ret_count, 1);
  ck_assert_int_eq(g_node_micstatus_list_ret_count, 0); // node wasn't touched
  ck_assert(!gs_mock_node->nd_nmics);
  }
END_TEST

START_TEST(pbs_read_json_mic_status_test_correct_statuses)
  {
  int rc;

  Json::Value status;
  Json::Value mic1;
  Json::Value mic2;

  mic1["micid"] = "mic1";
  mic1["key1"] = "1234567890";
  mic1["key2"] = "1234.00033";
  mic2["micid"] = "mic2";
  mic2["key1"] = "1234567890";
  mic2["key2"] = "1234.00033";

  status["mics"].append(mic1);
  status["mics"].append(mic2);

  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 3); // init only
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_node_micstatus_list_ret_count, 1); // node was updated
  g_decode_arst_ret_count = 0;
  g_free_arst_ret_count = 0;
  g_node_micstatus_list_ret_count = 0;

  status["mics"][0]["micid"] = "mic1";
  status["mics"][1]["micid"] = "mic2";
  status["mics"][0]["key1"] = 123;
  status["mics"][1]["key1"] = true;

  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 3); // init only
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_node_micstatus_list_ret_count, 1); // node was updated
  g_decode_arst_ret_count = 0;
  g_free_arst_ret_count = 0;
  g_node_micstatus_list_ret_count = 0;

  status["mics"][0]["micid"] = "mic1";
  status["mics"][1]["micid"] = "mic2";
  status["mics"][2]["micid"] = "mic3";
  status["mics"][2]["key1"] = "value1";

  rc = gs_test_helper->pbsReadJsonMicStatus(status);
  ck_assert_int_eq(rc, DIS_SUCCESS);
  ck_assert_int_eq(g_decode_arst_ret_count, 4); // init only
  ck_assert_int_eq(g_free_arst_ret_count, 0);
  ck_assert_int_eq(g_node_micstatus_list_ret_count, 1); // node was updated
  ck_assert_int_eq(gs_mock_node->nd_nmics_alloced, 3);
  ck_assert_int_eq(gs_mock_node->nd_nmics, 3);
  }
END_TEST

/* Testing function: MomUpdate::pbsReadJsonStatus()
 * Input parameters:
 *    const size_t sz - data buffer size
 *          check: fail if <= 0
 *    const shar * data - data buffer containing json status message
 *          check: fail if NULL, fail if can't parse
 * Output parameters:
 *    int, (ret code)
 *          check: -1 if fail, 0 if success
 * Calling functions:
 *    bool Json::Reader::parse() - return false if fail parse
 *          check: fail if false
 *    int get_svr_attr_l(int attr_index, long *l) - ret code ignored
 *          check: work with any returned values
 *    struct pbsnode *find_nodebyname(char *) - node or NULL
 *          check: do nothing if NULL
 *    mutex_mgr node_mutex(pthread_mutex_t *mutex, bool is_locked) - create mutex_mgr
 *          check: nothing
 *    int remove_hello(hello_container *hc, char *node_name)
 *          check: called if first_update is true
 *    int enqueue_threadpool_request(void *(*func)(void *), void *arg)
 *          check: called if first_update is true
 *    void clear_nvidia_gpus(struct pbsnode *np)
 *          check: called if first_update is true
 *    void update_node_state(struct pbsnode *np, int newstate)
 *          check: called if message=ERROR and down_on_error is set
 *    int process_state_str_val(struct pbsnode *np, const char *str)
 *          check: called if state present and !dont_change_state
 *    void update_job_data(struct pbsnode *np, const char *jobstring_in)
 *          check: called if jobdata present and mom_job_sync is set
 *    int handle_auto_np_val(struct pbsnode *np, const char *str)
 *          check: called if ncpus present
 *    int decode_arst() - return PBSE_NONE if success
 *          check: called with NULL for initialization. called after parsed each node
 *    void free_arst(pbs_attribute *) - free attribute data
 *          check: called once for each new attribute if not handled successfully
 *    int save_node_status(struct pbsnode *np, pbs_attribute *temp)
 *          check: called once for each successful parsed node
 */

void pbs_read_json_status_tc_setup(void)
  {
  gs_mock_node = init_node();
  gs_mom_update = new MomUpdate();
  gs_test_helper = new TestHelper(*gs_mom_update);

  allow_any_mom = 0;
  g_clear_nvidia_gpus_ret_count = 0;
  g_decode_arst_ret = 0;
  g_decode_arst_ret_count = 0;
  g_decode_arst_ret_map.clear();
  g_enqueue_threadpool_request_ret_count = 0;
  g_free_arst_ret_count = 0;
  g_find_nodebyname_ret = gs_mock_node;
  g_get_svr_attr_l_ret_mom_down_on_error = 0;
  g_get_svr_attr_l_ret_mom_job_sync = 0;
  g_handle_auto_np_val_ret_count = 0;
  g_process_state_str_val_ret_count = 0;
  g_process_uname_str_ret_count = 0;
  g_remove_hello_ret_count = 0;
  g_update_job_data_ret_count = 0;
  g_update_node_state_ret_count = 0;
  }

void pbs_read_json_status_tc_teardown(void)
  {
  deinit_node(gs_mock_node);
  g_decode_arst_ret_map.clear();
  delete(gs_test_helper);
  delete(gs_mom_update);
  }

START_TEST(pbs_read_json_status_test_null_value)
  {
  int rc;

  /* Check null size */
  rc = gs_mom_update->pbsReadJsonStatus(0, "{}");
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);

  /* Check null data */
  g_decode_arst_ret_count = 0;
  rc = gs_mom_update->pbsReadJsonStatus(10, NULL);
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);

  /* Check empty message */
  g_decode_arst_ret_count = 0;
  rc = gs_mom_update->pbsReadJsonStatus(3, "{}");
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);

  /* Check malformed data */
  g_decode_arst_ret_count = 0;
  rc = gs_mom_update->pbsReadJsonStatus(5, "asdf");
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_status_test_non_status_type)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;

  /* Check non-status message */
  root["messageType"] = "asdf";
  result = writer.write(root);

  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_status_test_non_array_body)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;

  /* Check missed body */
  root["messageType"] = "status";
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);

  /* Check non-array body */
  root["body"] = "asdf";
  g_decode_arst_ret_count = 0;
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_status_test_wrong_senderId)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["body"][0]["node"] = "node1";

  /* Check missed senderId */
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);

  /* Check non-string senderId */
  root["senderId"] = 23;
  g_decode_arst_ret_count = 0;
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_status_test_empty_body)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  Json::Value body(Json::arrayValue);
  root["body"] = body;

  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_node_without_id)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["key"] = "value";

  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_node_not_found)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  result = writer.write(root);

  g_find_nodebyname_ret = NULL;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_first_update_is_set)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  root["body"][0]["first_update"] = false;

  /* First update is set to false */
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 1); // wrote nothing
  ck_assert_int_eq(g_remove_hello_ret_count, 0);
  ck_assert_int_eq(g_enqueue_threadpool_request_ret_count, 0);
  ck_assert_int_eq(g_clear_nvidia_gpus_ret_count, 0);

  g_decode_arst_ret_count = 0;
  g_remove_hello_ret_count = 0;
  g_enqueue_threadpool_request_ret_count = 0;
  g_clear_nvidia_gpus_ret_count = 0;
  /* First update is set to non-bool value */
  root["body"][0]["node"] = "node1";
  root["body"][0]["first_update"] = 1;
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  ck_assert_int_eq(g_remove_hello_ret_count, 0);
  ck_assert_int_eq(g_enqueue_threadpool_request_ret_count, 0);
  ck_assert_int_eq(g_clear_nvidia_gpus_ret_count, 0);

  g_decode_arst_ret_count = 0;
  g_remove_hello_ret_count = 0;
  g_enqueue_threadpool_request_ret_count = 0;
  g_clear_nvidia_gpus_ret_count = 0;
  root["body"][0]["node"] = "node1";
  root["body"][0]["first_update"] = true;
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  ck_assert_int_eq(g_remove_hello_ret_count, 1);
  ck_assert_int_eq(g_enqueue_threadpool_request_ret_count, 1);
  ck_assert_int_eq(g_clear_nvidia_gpus_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_message_error)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  root["body"][0]["message"] = "ERROR";

  /* error message present, down_on_error is false */
  result = writer.write(root);
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_update_node_state_ret_count, 0);

  g_decode_arst_ret_count = 0;
  g_update_node_state_ret_count = 0;
  root["body"][0]["node"] = "node1";

  /* error message present, down_on_error is true */
  g_get_svr_attr_l_ret_mom_down_on_error = 1;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_update_node_state_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_state)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  root["body"][0]["state"] = "test state";
  result = writer.write(root);

  /* State is updated */
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_process_state_str_val_ret_count, 1);

  g_decode_arst_ret_count = 0;
  g_process_state_str_val_ret_count = 0;
  root["body"][0]["node"] = "node1";

  /* State isn't updated if dont_change_state */
  gs_mock_node->nd_mom_reported_down = 1;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_process_state_str_val_ret_count, 0);
  }
END_TEST

START_TEST(pbs_read_json_status_test_uname)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  root["body"][0]["uname"] = "test uname";
  result = writer.write(root);

  /* any mom disallowed */
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_process_uname_str_ret_count, 0);

  g_decode_arst_ret_count = 0;
  g_process_uname_str_ret_count = 0;
  root["body"][0]["node"] = "node1";

  /* State isn't updated if dont_change_state */
  allow_any_mom = 1;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_process_uname_str_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_jobdata)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  root["body"][0]["jobdata"] = "test jobdata";
  result = writer.write(root);

  /* jobdata presents, mom_job_sync property isn't set */
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_update_job_data_ret_count, 0);

  g_decode_arst_ret_count = 0;
  g_update_job_data_ret_count = 0;
  root["body"][0]["node"] = "node1";

  /* jobdata presents, mom_job_sync property is set */
  g_get_svr_attr_l_ret_mom_job_sync = 1;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_update_job_data_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_jobs)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  root["body"][0]["jobs"] = "test jobs";
  result = writer.write(root);

  /* jobs present, mom_job_sync property isn't set */
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_enqueue_threadpool_request_ret_count, 0);

  g_decode_arst_ret_count = 0;
  g_update_job_data_ret_count = 0;
  root["body"][0]["node"] = "node1";

  /* jobs present, mom_job_sync property is set */
  g_get_svr_attr_l_ret_mom_job_sync = 1;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_enqueue_threadpool_request_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_ncpus)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  root["body"][0]["ncpus"] = "123";
  result = writer.write(root);

  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  ck_assert_int_eq(g_handle_auto_np_val_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_attribute_init_fail)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  
  root["body"][0]["availmem"] = "asdf";
  root["body"][0]["idletime"] = "asdf";
  root["body"][0]["netload"] = "asdf";
  root["body"][0]["nsessions"] = "asdf";
  root["body"][0]["nusers"] = "asdf";
  root["body"][0]["totmem"] = "asdf";
  root["body"][0]["physmem"] = "asdf";
  root["body"][0]["loadave"] = "asdf";
  root["body"][0]["opsys"] = "asdf";
  root["body"][0]["gres"] = "asdf";
  root["body"][0]["varattr"] = "asdf";
  root["body"][0]["sessions"] = "asdf";

  result = writer.write(root);

  g_decode_arst_ret = -1;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, -1);
  ck_assert_int_eq(g_decode_arst_ret_count, 1);
  }
END_TEST

START_TEST(pbs_read_json_status_test_attribute_decode_fail)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  
  root["body"][0]["availmem"] = "asdf";
  root["body"][0]["idletime"] = "asdf";
  root["body"][0]["netload"] = "asdf";
  root["body"][0]["nsessions"] = "asdf";
  root["body"][0]["nusers"] = "asdf";
  root["body"][0]["totmem"] = "asdf";
  root["body"][0]["physmem"] = "asdf";
  root["body"][0]["loadave"] = "asdf";
  root["body"][0]["opsys"] = "asdf";
  root["body"][0]["gres"] = "asdf";
  root["body"][0]["varattr"] = "asdf";
  root["body"][0]["sessions"] = "asdf";

  result = writer.write(root);

  g_decode_arst_ret_map[2] = -1;
  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  }
END_TEST

START_TEST(pbs_read_json_status_test_other_values)
  {
  int rc;
  Json::Value root;
  Json::FastWriter writer;
  std::string result;
  root["messageType"] = "status";
  root["senderId"] = "node1";
  root["body"][0]["node"] = "node1";
  
  root["body"][0]["availmem"] = "asdf";
  root["body"][0]["idletime"] = "asdf";
  root["body"][0]["netload"] = "asdf";
  root["body"][0]["nsessions"] = "asdf";
  root["body"][0]["nusers"] = "asdf";
  root["body"][0]["totmem"] = "asdf";
  root["body"][0]["physmem"] = "asdf";
  root["body"][0]["loadave"] = "asdf";
  root["body"][0]["opsys"] = "asdf";
  root["body"][0]["gres"] = "asdf";
  root["body"][0]["varattr"] = "asdf";
  root["body"][0]["sessions"] = "asdf";

  result = writer.write(root);

  rc = gs_mom_update->pbsReadJsonStatus(result.length(), result.c_str());
  ck_assert_int_eq(rc, 0);
  ck_assert_int_eq(g_decode_arst_ret_count, 2);
  }
END_TEST

Suite *zmq_process_mom_update_suite(void)
  {
  Suite *s = suite_create("zmq_process_mom_update_suite methods");
  TCase *tc_core;
  
  tc_core = tcase_create("pbs_read_json_gpu_status_tc");
  tcase_add_checked_fixture(tc_core, pbs_read_json_gpu_status_tc_setup,
      pbs_read_json_gpu_status_tc_teardown);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_null_value);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_non_array_value);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_attribute_init_fail);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_attribute_decode_fail);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_empty_array);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_no_gpuid);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_no_gpuidx_found);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_correct_statuses);
  tcase_add_test(tc_core, pbs_read_json_gpu_status_test_bad_status_values);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("pbs_read_json_mic_status_tc");
  tcase_add_checked_fixture(tc_core, pbs_read_json_mic_status_tc_setup,
      pbs_read_json_mic_status_tc_teardown);
  tcase_add_test(tc_core, pbs_read_json_mic_status_test_null_value);
  tcase_add_test(tc_core, pbs_read_json_mic_status_test_non_array_value);
  tcase_add_test(tc_core, pbs_read_json_mic_status_test_attribute_init_fail);
  tcase_add_test(tc_core, pbs_read_json_mic_status_test_attribute_decode_fail);
  tcase_add_test(tc_core, pbs_read_json_mic_status_test_empty_array);
  tcase_add_test(tc_core, pbs_read_json_mic_status_test_no_micid);
  tcase_add_test(tc_core, pbs_read_json_mic_status_test_correct_statuses);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("pbs_read_json_status_tc");
  tcase_add_checked_fixture(tc_core, pbs_read_json_status_tc_setup,
      pbs_read_json_status_tc_teardown);
  tcase_add_test(tc_core, pbs_read_json_status_test_null_value);
  tcase_add_test(tc_core, pbs_read_json_status_test_non_status_type);
  tcase_add_test(tc_core, pbs_read_json_status_test_non_array_body);
  tcase_add_test(tc_core, pbs_read_json_status_test_wrong_senderId);
  tcase_add_test(tc_core, pbs_read_json_status_test_empty_body);
  tcase_add_test(tc_core, pbs_read_json_status_test_node_without_id);
  tcase_add_test(tc_core, pbs_read_json_status_test_node_not_found);
  tcase_add_test(tc_core, pbs_read_json_status_test_first_update_is_set);
  tcase_add_test(tc_core, pbs_read_json_status_test_message_error);
  tcase_add_test(tc_core, pbs_read_json_status_test_state);
  tcase_add_test(tc_core, pbs_read_json_status_test_uname);
  tcase_add_test(tc_core, pbs_read_json_status_test_jobdata);
  tcase_add_test(tc_core, pbs_read_json_status_test_jobs);
  tcase_add_test(tc_core, pbs_read_json_status_test_ncpus);
  tcase_add_test(tc_core, pbs_read_json_status_test_attribute_init_fail);
  tcase_add_test(tc_core, pbs_read_json_status_test_attribute_decode_fail);
  tcase_add_test(tc_core, pbs_read_json_status_test_other_values);
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
  sr = srunner_create(zmq_process_mom_update_suite());
  srunner_set_log(sr, "zmq_process_mom_update_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  }
