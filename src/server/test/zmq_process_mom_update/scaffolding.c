#include "pbs_config.h"
#include "license_pbs.h" /* See here for the software license */

#include <pthread.h>

#include "attribute.h"
#include "mutex_mgr.hpp"
#include "log.h"
#include "pbs_nodes.h"
#include "server.h"

int             allow_any_mom = FALSE;
hello_container hellos;
int             LOGLEVEL = 10;
char            log_buffer[LOG_BUF_SIZE];

int g_decode_arst_ret;
int g_decode_arst_ret_count;
int g_free_arst_ret_count;
int g_node_gpustatus_list_ret;
int g_node_gpustatus_list_ret_count;
int g_update_nodes_file_ret_count;
int g_update_nodes_file_ret;
int g_gpu_entry_by_id_ret;
int g_gpu_has_job_ret;
int g_node_micstatus_list_ret_count;
int g_node_micstatus_list_ret;
int g_clear_nvidia_gpus_ret_count;
int g_enqueue_threadpool_request_ret_count;
int g_handle_auto_np_val_ret_count;
int g_process_state_str_val_ret_count;
int g_process_uname_str_ret_count;
int g_remove_hello_ret_count;
int g_update_job_data_ret_count;
int g_update_node_state_ret_count;

struct pbsnode * g_find_nodebyname_ret;
long g_get_svr_attr_l_ret_mom_down_on_error;
long g_get_svr_attr_l_ret_mom_job_sync;
long g_get_svr_attr_l_ret_auto_node_np;

int decode_arst(pbs_attribute *patr, const char *name, const char *rescn, const char *val, int perm)
  {
  g_decode_arst_ret_count++;
  return g_decode_arst_ret;
  }

void free_arst(pbs_attribute *attr)
  {
  g_free_arst_ret_count++;
  }

int node_gpustatus_list(pbs_attribute *new_attr, void *pnode, int actmode)
  {
  g_node_gpustatus_list_ret_count++;
  return g_node_gpustatus_list_ret;
  }

int update_nodes_file(struct pbsnode *held)
  {
  g_update_nodes_file_ret_count++;
  return g_update_nodes_file_ret;
  }

int enqueue_threadpool_request(void *(*func)(void *), void *arg)
  {
  g_enqueue_threadpool_request_ret_count++;
  return 0;
  }

void log_err(int errnum, const char *routine, const char *text)
  {
  // Do nothing, assume never fail
  }

void log_ext(int errnum, const char *routine, const char *text, int severity)
  {
  }

void log_event(int eventtype, int objclass, const char *objname, const char *text)
  {
  }

void log_record(int eventtype, int objclass, const char *objname, const char *text)
  {
  }

int node_micstatus_list(pbs_attribute *new_attr, void *pnode, int actmode)
  {
  g_node_micstatus_list_ret_count++;
  return g_node_micstatus_list_ret;
  }

int gpu_has_job(struct pbsnode *pnode, int gpuid)
  {
  return g_gpu_has_job_ret;
  }

int process_uname_str(struct pbsnode *np, const char *str)
  {
  g_process_uname_str_ret_count++;
  return 0;
  }

int remove_hello(hello_container *hc, char *node_name)
  {
  g_remove_hello_ret_count++;
  return 0;
  }

int process_state_str_val(struct pbsnode *np, const char *str)
  {
  g_process_state_str_val_ret_count++;
  return 0;
  }

void *sync_node_jobs(void *vp)
  {
  return NULL;
  }

int handle_auto_np_val(struct pbsnode *np, const char *str)
  {
  g_handle_auto_np_val_ret_count++;
  return 0;
  }

void update_job_data(struct pbsnode *np, const char *jobstring_in)
  {
  g_update_job_data_ret_count++;
  }

struct pbsnode *find_nodebyname(const char *nodename)
  {
  return g_find_nodebyname_ret;
  }

int save_node_status(struct pbsnode *np, pbs_attribute *temp)
  {
  return 0;
  }

int gpu_entry_by_id(struct pbsnode *pnode, const char *gpuid, int get_empty)
  {
  return g_gpu_entry_by_id_ret;
  }

void update_node_state(struct pbsnode *np, int newstate)
  {
  g_update_node_state_ret_count++;
  }

void clear_nvidia_gpus(struct pbsnode *np)
  {
  g_clear_nvidia_gpus_ret_count++;
  }

int get_svr_attr_l(int attr_index, long *l)
  {
  switch (attr_index)
    {
    case SRV_ATR_MomJobSync:
      *l = g_get_svr_attr_l_ret_mom_job_sync;
      break;
    case SRV_ATR_AutoNodeNP:
      *l = g_get_svr_attr_l_ret_auto_node_np;
      break;
    case SRV_ATR_DownOnError:
      *l = g_get_svr_attr_l_ret_mom_down_on_error;
      break;
    default:
      break;
    }
  return 0;
  }

void *send_hierarchy_threadtask(void *vp)
  {
  return NULL;
  }

mutex_mgr::mutex_mgr(pthread_mutex_t *mutex, bool is_locked)
  {
  // Do nothing
  }

mutex_mgr::~mutex_mgr()
  {
  // Do nothing
  }

int mutex_mgr::unlock()
  {
  return 0;
  }
