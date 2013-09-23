#include <pbs_config.h>

#ifdef ZMQ

#include <sstream>
#include <jsoncpp/json/json.h>
#include <stdio.h>
#include <errno.h>

#include "log.h"
#include "dynamic_string.h"
#include "pbs_nodes.h"
#include "attribute.h"
// TODO: use non-dis errors and remove include of dis.h
#include "dis.h"
#include "server.h"
#include "svrfunc.h"
#include "mutex_mgr.hpp"
#include "threadpool.h"

#include "zmq_process_mom_update.h"
#include "MomStatusMessage.hpp"

extern int              allow_any_mom;

int gpu_entry_by_id(struct pbsnode *,const char *, int);
int gpu_has_job(struct pbsnode *pnode, int gpuid);
int save_single_mic_status(dynamic_string *single_mic_status, pbs_attribute *temp);
void clear_nvidia_gpus(struct pbsnode *np);
int process_state_str_val(struct pbsnode *np, const char *str);
int process_uname_str(struct pbsnode *np, const char *str);
int handle_auto_np_val(struct pbsnode *np, const char *str);
int save_node_status(struct pbsnode *current, pbs_attribute *temp);
void update_job_data(struct pbsnode *np, const char *jobstring_in);


namespace TrqZStatus {

int MomUpdate::pbsReadJsonStatus(const size_t sz, const char *data)
  {
  long              mom_job_sync = 0;
  long              auto_np = 0;
  long              down_on_error = 0;
  Json::Value       root;
  std::stringstream status_stream;
  Json::Value      *temp_value;
  pbs_attribute     temp;
  int               rc;

  if (LOGLEVEL >= 10)
    {
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__,
        "Reading of JSON status");
    }

  if (sz <= 0 || !data)
    {
    return -1;
    }

  // read
  bool parsingSuccessful = m_reader.parse(data, data + sz - 1, root, false);
  if ( !parsingSuccessful )
    {
    // Can't read the message. Malformed?
    log_err(-1, __func__, "malformed Json message received");
    return -1;
    }

  temp_value = &root["messageType"];
  if (temp_value->isNull() || !temp_value->isString() || temp_value->asString() != "status")
    {
    // There is no 'messageType' key in the message
    log_err(-1, __func__, "non-status message received");
    return -1;
    }

  Json::Value &body = root["body"];

  if (body.isNull() || !body.isArray())
    {
    // Body have to be an array of nodes statuses
    log_err(-1, __func__, "status message containg no body");
    return -1;
    }

  temp_value = &root[JSON_SENDER_ID_KEY];
  if (!temp_value->isString())
    {
    return -1;
    }
  sprintf(log_buffer, "Got json statuses message from senderId:%s", temp_value->asString().c_str());
  log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);

  get_svr_attr_l(SRV_ATR_MomJobSync, &mom_job_sync);
  get_svr_attr_l(SRV_ATR_AutoNodeNP, &auto_np);
  get_svr_attr_l(SRV_ATR_DownOnError, &down_on_error);

  /* Before filling the "temp" pbs_attribute, initialize it.
   * The second and third parameter to decode_arst are never
   * used, so just leave them empty. (GBS) */
  memset(&temp, 0, sizeof(temp));

  if ((rc = decode_arst(&temp, NULL, NULL, NULL, 0)) != PBSE_NONE)
    {
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, "cannot initialize attribute");
    return(rc);
    }

  // TODO: Handle NUMA status field.
  for (Json::ValueIterator i = body.begin(); i != body.end(); i++)
    {
    Json::Value &node_status = *i;
    bool dont_change_state = false;

    Json::Value node_id_val = node_status.removeMember("node");
    if (node_id_val.isNull() || !node_id_val.isString())
      {
      log_err(-1, __func__, "received a status without node id specified. Ignored");
      continue;
      }
    std::string nodeId = node_id_val.asString();

    sprintf(log_buffer, "Got status nodeId:%s", nodeId.c_str());
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);

    m_current_node = find_nodebyname(nodeId.c_str());
    if(m_current_node == NULL)
      {
      // TODO: Check is the node trusted and create an entinty for the node if not found.
      //       See svr_is_request() code.
      sprintf(log_buffer, "the node with id '%s' not found. Ignored", nodeId.c_str());
      log_err(-1, __func__, log_buffer);
      continue;
      }

    mutex_mgr node_mutex(m_current_node->nd_mutex, true);

    if (LOGLEVEL >= 10)
      {
      snprintf(log_buffer, LOCAL_LOG_BUF_SIZE, "handle status for node '%s'", nodeId.c_str());
      log_event(PBSEVENT_ADMIN,PBS_EVENTCLASS_SERVER,__func__,log_buffer);
      }

    if (m_current_node->nd_mom_reported_down)
      {
      dont_change_state = true;
      }

    for (Json::ValueIterator j = node_status.begin(); j != node_status.end(); j++)
      {
      std::string key = j.memberName();
      temp_value = &(*j);

      if (temp_value->isNull())
        {
        continue;
        }

      /* Check special values like gpu, mic statuses and values that aren't node attributes */
      if (!key.compare(GPU_STATUS_KEY))
        {
        pbsReadJsonGpuStatus(*temp_value);
        continue;
        }
      else if (!key.compare(MIC_STATUS_KEY))
        {
        pbsReadJsonMicStatus(*temp_value);
        continue;
        }
      else if (!key.compare("first_update"))
        {
        if (temp_value->isBool())
          {
          if (LOGLEVEL >= 10)
            {
            log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__,
                ("status: first_update = " + temp_value->asString()).c_str());
            }

          if (temp_value->asBool())
            {
            /* mom is requesting that we send the mom hierarchy file to her */
            remove_hello(&hellos, m_current_node->nd_name);      

            /* send hierarchy to mom*/ 
            struct hello_info *hi = (struct hello_info *)calloc(1, sizeof(struct hello_info));

            hi->name = strdup(m_current_node->nd_name);
            enqueue_threadpool_request(send_hierarchy_threadtask, hi);        

            /* reset gpu data in case mom reconnects with changed gpus */
            clear_nvidia_gpus(m_current_node);
            }
          }
        continue;
        } /* end of first_update key */

      /* Add a value to the node attributes list */
      if (temp_value->isString() || temp_value->isBool())
        {
        status_stream << key << '=' << temp_value->asString() << ';';
        }
      else if (temp_value->isInt())
        {
        status_stream << key << '=' << temp_value->asInt() << ';';
        }
      else
        {
        if (LOGLEVEL >= 10)
          {
          log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__,
              ("status: cannot add attribute " + key + '=' + temp_value->toStyledString()).c_str());
          }
        continue;
        }

      /* Perform special actions with attribute */
      if (!key.compare("state"))
        {
        if (temp_value->isString() && !dont_change_state)
          {
            process_state_str_val(m_current_node, temp_value->asCString());
          }
        }
      else if (!key.compare("uname"))
        {
        if (temp_value->isString() && allow_any_mom)
          {
          process_uname_str(m_current_node, temp_value->asCString());
          }
        }
      else if (!key.compare("message"))
        {
        if (temp_value->isString() && !temp_value->asString().compare("ERROR") && down_on_error)
          {
          update_node_state(m_current_node, INUSE_DOWN);
          dont_change_state = true;
          }
        }
      else if (!key.compare("jobdata"))
        {
        if (temp_value->isString() && mom_job_sync)
          {
          update_job_data(m_current_node, temp_value->asCString());
          }
        }
      else if (!key.compare("jobs"))
        {
        if (temp_value->isString() && mom_job_sync)
          {
          std::string value = temp_value->asString();
          size_t len = value.length() + strlen(m_current_node->nd_name) + 2;
          char *jobstr = (char *)calloc(1, len);
          sync_job_info *sji = (sync_job_info *)calloc(1, sizeof(sync_job_info));

          if (jobstr == NULL || sji == NULL)
            {
            if (jobstr != NULL)
              {
              free(jobstr);
              jobstr = NULL;
              }
            if (sji != NULL)
              {
              free(sji);
              sji = NULL;
              }
            }
          else
            {
            snprintf(jobstr, len, "%s:%s", m_current_node->nd_name, value.c_str());
            sji->input = jobstr;
            sji->timestamp = time(NULL);

            /* sji must be freed in sync_node_jobs */
            enqueue_threadpool_request(sync_node_jobs, sji);
            }
          }
        }
      else if (!key.compare("ncpus"))
        {
        if (temp_value->isString())
          {
          handle_auto_np_val(m_current_node, temp_value->asCString());
          }
        }
      } /* END processing node status values */

    if (status_stream.rdbuf()->in_avail())
      {
      // Put collected values into the temp attribute
      if (decode_arst(&temp, NULL, NULL, status_stream.str().c_str(), 0))
        {
        DBPRT(("mom_read_json_status: cannot add attributes\n"));
        free_arst(&temp);
        }
      else
        {
        save_node_status(m_current_node, &temp);
        }

      /* reset the status_stream for the next iteration */
      status_stream.str("");
      status_stream.clear();
      }

    node_mutex.unlock();
    m_current_node = NULL;
    } /* END for status["body"] */

  return(DIS_SUCCESS);
  } /* END mom_read_json_status() */

int MomUpdate::pbsReadJsonGpuStatus(Json::Value &gpus_status)
  {
  pbs_attribute     temp;
  std::stringstream gpuinfo_stream;
  unsigned int      startgpucnt;
  int               rc;
  const Json::Value *temp_value;
  int               drv_ver;

  if (LOGLEVEL >= 7)
    {
    sprintf(log_buffer, "received gpu status from node %s", m_current_node->nd_name);
    log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buffer);
    }

  /* Check input */
  if (gpus_status.isNull() || !gpus_status.isObject())
    {
    return(DIS_NOCOMMIT);
    }

  Json::Value &gpus_array = gpus_status["gpus"];
  if (!gpus_array.isArray())
    {
    return(DIS_NOCOMMIT);
    }

  /* save current gpu count for node */
  startgpucnt = m_current_node->nd_ngpus;

  /*
   *  Before filling the "temp" pbs_attribute, initialize it.
   *  The second and third parameter to decode_arst are never
   *  used, so just leave them empty. (GBS)
   */
  memset(&temp, 0, sizeof(temp));
  rc = decode_arst(&temp, NULL, NULL, NULL, 0);
  if (rc != PBSE_NONE)
    {
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, "cannot initialize attribute");
    return(DIS_NOCOMMIT);
    }

  /* Process timestamp value */
  temp_value = &gpus_status["timestamp"];
  if (temp_value->isString())
    {
    if (decode_arst(&temp, NULL, NULL, temp_value->asCString(), 0))
      {
      DBPRT(("pbsReadJsonGpuStatus: cannot add attributes\n"));
      free_arst(&temp);
      return(DIS_NOCOMMIT);
      }
    }

  temp_value = &gpus_status["driver_ver"];
  if (!temp_value->isNull())
    {
    std::stringstream drv_ver_str;
    if (temp_value->isString())
      {
      drv_ver = atoi(temp_value->asCString());
      drv_ver_str << temp_value->asString();
      }
    else if (temp_value->isInt())
      {
      drv_ver = temp_value->asInt();
      drv_ver_str << drv_ver;
      }

    if (drv_ver_str.rdbuf()->in_avail())
      {
      if (decode_arst(&temp, NULL, NULL, drv_ver_str.str().c_str(), 0))
        {
        DBPRT(("pbsReadJsonGpuStatus: cannot add attributes\n"));
        free_arst(&temp);
        return(DIS_NOCOMMIT);
        }
      }
    }

  for (Json::ValueIterator i = gpus_array.begin(); i != gpus_array.end(); i++)
    {
    Json::Value &gpu_status = *i;
    std::string gpuid;
    int gpuidx = -1;

    /* get gpuid */
    Json::Value remove_member = gpu_status.removeMember("gpuid");
    if (remove_member.isNull() || !remove_member.isString())
      {
      if (LOGLEVEL >= 3)
        {
        sprintf(log_buffer,
            "Failed to get/create entry for gpu without gpuid specified on node %s\n",
            m_current_node->nd_name);

        log_ext(-1, __func__, log_buffer, LOG_DEBUG);
        }

      free_arst(&temp);
      return(DIS_NOCOMMIT);
      }
    gpuid = remove_member.asString();

    /*
     * Get this gpus index, if it does not yet exist then find an empty entry.
     * We need to allow for the gpu status results being returned in
     * different orders since the nvidia order may change upon mom's reboot
     */
    gpuidx = gpu_entry_by_id(m_current_node, gpuid.c_str(), TRUE);
    if (gpuidx == -1)
      {
      /*
       * Failure - we could not get / create a nd_gpusn entry for this gpu,
       * log an error message.
       */

      if (LOGLEVEL >= 3)
        {
        sprintf(log_buffer,
            "Failed to get/create entry for gpu %s on node %s\n",
            gpuid.c_str(),
            m_current_node->nd_name);

        log_ext(-1, __func__, log_buffer, LOG_DEBUG);
        }

      free_arst(&temp);

      /*
       * Don't know the reason why we return success here.
       * It's taken from the original logic from proces_mom_update.c:is_gpustat_get().
       */
      return(DIS_SUCCESS);
      }

    gpuinfo_stream << "gpu[" << gpuidx << "]=gpu_id=" << gpuid;

    m_current_node->nd_gpusn[gpuidx].driver_ver = drv_ver;

    /* mark that this gpu node is not virtual */
    m_current_node->nd_gpus_real = TRUE;

    /*
     * if we have not filled in the gpu_id returned by the mom node
     * then fill it in
     */
    if ((gpuidx >= 0) && (m_current_node->nd_gpusn[gpuidx].gpuid == NULL))
      {
      m_current_node->nd_gpusn[gpuidx].gpuid = strdup(gpuid.c_str());
      }

    for (Json::ValueIterator j = gpu_status.begin(); j != gpu_status.end(); j++)
      {
      std::string key = j.memberName();
      temp_value = &(*j);

      if (temp_value->isNull())
        {
        continue;
        }

      if (!key.compare("gpu_mode"))
        {
        if (!temp_value->isNull() && temp_value->isString())
          {
          std::string mode = temp_value->asString();
          if (!mode.compare("Normal") || !mode.compare("Default"))
            {
            m_current_node->nd_gpusn[gpuidx].mode = gpu_normal;
            if (gpu_has_job(m_current_node, gpuidx))
              {
              m_current_node->nd_gpusn[gpuidx].state = gpu_shared;
              }
            else
              {
              m_current_node->nd_gpusn[gpuidx].inuse = 0;
              m_current_node->nd_gpusn[gpuidx].state = gpu_unallocated;
              }
            }
          else if (!mode.compare("Exclusive") ||
              !mode.compare("Exclusive_Thread"))
            {
            m_current_node->nd_gpusn[gpuidx].mode = gpu_exclusive_thread;
            if (gpu_has_job(m_current_node, gpuidx))
              {
              m_current_node->nd_gpusn[gpuidx].state = gpu_exclusive;
              }
            else
              {
              m_current_node->nd_gpusn[gpuidx].inuse = 0;
              m_current_node->nd_gpusn[gpuidx].state = gpu_unallocated;
              }
            }
          else if (!mode.compare("Exclusive_Process"))
            {
            m_current_node->nd_gpusn[gpuidx].mode = gpu_exclusive_process;
            if (gpu_has_job(m_current_node, gpuidx))
              {
              m_current_node->nd_gpusn[gpuidx].state = gpu_exclusive;
              }
            else
              {
              m_current_node->nd_gpusn[gpuidx].inuse = 0;
              m_current_node->nd_gpusn[gpuidx].state = gpu_unallocated;
              }
            }
          else if (!mode.compare("Prohibited"))
            {
            m_current_node->nd_gpusn[gpuidx].mode = gpu_prohibited;
            m_current_node->nd_gpusn[gpuidx].state = gpu_unavailable;
            }
          else
            {
            /* unknown mode, default to prohibited */
            m_current_node->nd_gpusn[gpuidx].mode = gpu_prohibited;
            m_current_node->nd_gpusn[gpuidx].state = gpu_unavailable;
            if (LOGLEVEL >= 3)
              {
              sprintf(log_buffer,
                  "GPU %s has unknown mode on node %s",
                  gpuid.c_str(),
                  m_current_node->nd_name);

              log_ext(-1, __func__, log_buffer, LOG_DEBUG);
              }
            }

          /* add gpu_mode so it gets added to the pbs_attribute */
          gpuinfo_stream << ";gpu_state=";
          switch (m_current_node->nd_gpusn[gpuidx].state)
            {
            case gpu_unallocated:
              gpuinfo_stream << "Unallocated";
              break;
            case gpu_shared:
              gpuinfo_stream << "Shared";
              break;
            case gpu_exclusive:
              gpuinfo_stream << "Exclusive";
              break;
            case gpu_unavailable:
              gpuinfo_stream << "Unavailable";
              break;
            }
          }
        // We're already pushed the string representation into the gpuinfo_stream and have to
        // continue.
        continue;
        }

      if (temp_value->isString() || temp_value->isBool())
        {
        gpuinfo_stream << ';' << key << '=' << temp_value->asString();
        }
      else if (temp_value->isInt())
        {
        gpuinfo_stream << ';' << key << '=' << temp_value->asInt();
        }

      } /* END of for single gpu status keys */

    if (decode_arst(&temp, NULL, NULL, gpuinfo_stream.str().c_str(), 0))
      {
      DBPRT(("is_gpustat_get: cannot add attributes\n"));

      free_arst(&temp);

      return(DIS_NOCOMMIT);
      }

    /* reset the gpuinfo stream for the next iteration */
    gpuinfo_stream.str("");
    gpuinfo_stream.clear();

    } /* END of for GPUs */

  /* maintain the gpu count, if it has changed we need to update the nodes file */
  if (gpus_array.size() != startgpucnt)
    {
    m_current_node->nd_ngpus = gpus_array.size();

    /* update the nodes file */
    update_nodes_file(m_current_node);
    }

  node_gpustatus_list(&temp, m_current_node, ATR_ACTION_ALTER);

  return(DIS_SUCCESS);
  }  /* END pbs_read_json_gpu_status() */


int MomUpdate::pbsReadJsonMicStatus(Json::Value &mics_status)
  {
  pbs_attribute     temp;
  std::stringstream micinfo_stream;
  int               mic_count = 0;
  int               rc = PBSE_NONE;

  if (LOGLEVEL >= 7)
    {
    sprintf(log_buffer, "received mic status from node %s",
        (m_current_node != NULL) ? m_current_node->nd_name : "NULL");
    log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buffer);
    }

  if (mics_status.isNull() || !mics_status.isObject())
    {
    return(DIS_NOCOMMIT);
    }

  Json::Value &mics_array = mics_status["mics"];
  if (!mics_array.isArray())
    {
    return(DIS_NOCOMMIT);
    }

  /*
   *  Before filling the "temp" pbs_attribute, initialize it.
   *  The second and third parameter to decode_arst are never
   *  used, so just leave them empty. (GBS)
   */
  memset(&temp, 0, sizeof(temp));
  rc = decode_arst(&temp, NULL, NULL, NULL, 0);
  if (rc != PBSE_NONE)
    {
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, "cannot initialize attribute");
    return(DIS_NOCOMMIT);
    }

  for (Json::ValueIterator i = mics_array.begin(); i != mics_array.end(); i++)
    {
    Json::Value &mic_status = *i;
    const Json::Value *temp_value;
    std::string micid;

    /* add the info to the "temp" attribute */

    /* get micid */
    Json::Value micid_val = mic_status.removeMember("micid");
    // TODO: Null value isn't string, simplify the checks
    if (micid_val.isNull() || !micid_val.isString())
      {
      if (LOGLEVEL >= 3)
        {
        sprintf(log_buffer,
            "Failed to get/create entry for mic without micid specified on node %s\n",
            m_current_node->nd_name);

        log_ext(-1, __func__, log_buffer, LOG_DEBUG);
        }

      free_arst(&temp);
      return(DIS_NOCOMMIT);
      }
    micid = micid_val.asString();

    micinfo_stream << "mic[" << mic_count << "]=" << micid;

    // Print all other mic status values
    for (Json::ValueIterator j = mic_status.begin(); j != mic_status.end(); j++)
      {
      std::string key = j.memberName();
      temp_value = &(*j);

      if (temp_value->isString() || temp_value->isBool())
        {
        micinfo_stream << ';' << key << '=' << temp_value->asString();
        }
      else if (temp_value->isInt())
        {
        micinfo_stream << ';' << key << '=' << temp_value->asInt();
        }
      }

    // TODO: lookup decode_arst code, it should be better to have a special function to add one key-value pair to attribute.
    rc = decode_arst(&temp, NULL, NULL, micinfo_stream.str().c_str(), 0);
    if (rc != PBSE_NONE)
      {
      log_err(ENOMEM, __func__, "error decoding mic status");
      free_arst(&temp);
      break;
      }

    mic_count++;

    /* reset the micinfo stream for the next iteration */
    micinfo_stream.str("");
    micinfo_stream.clear();
    
    } /* END */

  if (mic_count > m_current_node->nd_nmics)
    {
    m_current_node->nd_nmics_free += mic_count - m_current_node->nd_nmics;
    m_current_node->nd_nmics = mic_count;

    if (mic_count > m_current_node->nd_nmics_alloced)
      {
      struct jobinfo *tmp = (struct jobinfo *)calloc(mic_count, sizeof(struct jobinfo));

      if (tmp == NULL)
        return(ENOMEM);

      memcpy(tmp, m_current_node->nd_micjobs, sizeof(struct jobinfo) * m_current_node->nd_nmics_alloced);
      free(m_current_node->nd_micjobs);
      m_current_node->nd_micjobs = tmp;

      m_current_node->nd_nmics_alloced = mic_count;
      }
    }

  node_micstatus_list(&temp, m_current_node, ATR_ACTION_ALTER);

  return(rc);
  }

} /* END namespace TrqZStatus */

#endif /* ZMQ */
