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


/**
 * Read GPU status from JsonCPP object into the given node attributes.
 * @param np the pointer to PBS node to be updated.
 * @param gpus_status the Json value object reference containing GPU status.
 * @return DIS_SUCCESS (0) if succeeded or DIS_NOCOMMIT otherwise.
 */
int pbs_read_json_gpu_status(struct pbsnode *np, const Json::Value &gpus_status)
  {
  pbs_attribute     temp;
  std::stringstream gpuinfo_stream;
  unsigned int      startgpucnt;
  bool              gpus_real;
  int               rc;

  if (LOGLEVEL >= 7)
    {
    sprintf(log_buffer, "received gpu status from node %s", np->nd_name);
    log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buffer);
    }

  /* Check input */
  if (gpus_status.isNull() || !gpus_status.isArray())
    {
    return(DIS_NOCOMMIT);
    }

  /* save current gpu count for node */
  startgpucnt = np->nd_ngpus;

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

  gpus_real = false;
  for (unsigned int i = 0; i < gpus_status.size(); i++)
    {
    const Json::Value &gpu_status = gpus_status[i];
    const Json::Value *temp_value;
    std::string gpuid;
    int gpuidx = -1;

    /* add the info to the "temp" attribute */

    /* get gpuid */
    temp_value = &gpu_status["gpuid"];
    if (temp_value->isNull() || !temp_value->isString())
      {
      if (LOGLEVEL >= 3)
        {
        sprintf(log_buffer,
            "Failed to get/create entry for gpu without gpuid specified on node %s\n",
            np->nd_name);

        log_ext(-1, __func__, log_buffer, LOG_DEBUG);
        }

      free_arst(&temp);
      return(DIS_NOCOMMIT);
      }
    gpuid = temp_value->asString();

    /*
     * Get this gpus index, if it does not yet exist then find an empty entry.
     * We need to allow for the gpu status results being returned in
     * different orders since the nvidia order may change upon mom's reboot
     */
    gpuidx = gpu_entry_by_id(np, gpuid.c_str(), TRUE);
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
            np->nd_name);

        log_ext(-1, __func__, log_buffer, LOG_DEBUG);
        }

      free_arst(&temp);

      return(DIS_SUCCESS);
      }

    gpuinfo_stream << "gpu[" << gpuidx << "]=gpu_id=" << gpuid;
    /*
     * if we have not filled in the gpu_id returned by the mom node
     * then fill it in
     */
    if ((gpuidx >= 0) && (np->nd_gpusn[gpuidx].gpuid == NULL))
      {
      np->nd_gpusn[gpuidx].gpuid = strdup(gpuid.c_str());
      }      

    /* get timestamp */
    temp_value = &gpu_status["timestamp"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      gpuinfo_stream << ";timestamp=" << temp_value->asString();
      }

    /* get driver version, if there is one */
    temp_value = &gpu_status["driver_ver"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      gpuinfo_stream << ";driver_ver=" << temp_value->asString();

      np->nd_gpusn[gpuidx].driver_ver = atoi(temp_value->asCString());
      }

    temp_value = &gpu_status["gpu_mode"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      std::string mode = temp_value->asString();
      if (!mode.compare("Normal") || !mode.compare("Default"))
        {
        np->nd_gpusn[gpuidx].mode = gpu_normal;
        if (gpu_has_job(np, gpuidx))
          {
          np->nd_gpusn[gpuidx].state = gpu_shared;
          }
        else
          {
          np->nd_gpusn[gpuidx].inuse = 0;
          np->nd_gpusn[gpuidx].state = gpu_unallocated;
          }
        }
      else if (!mode.compare("Exclusive") ||
               !mode.compare("Exclusive_Thread"))
        {
        np->nd_gpusn[gpuidx].mode = gpu_exclusive_thread;
        if (gpu_has_job(np, gpuidx))
          {
          np->nd_gpusn[gpuidx].state = gpu_exclusive;
          }
        else
          {
          np->nd_gpusn[gpuidx].inuse = 0;
          np->nd_gpusn[gpuidx].state = gpu_unallocated;
          }
        }
      else if (!mode.compare("Exclusive_Process"))
        {
        np->nd_gpusn[gpuidx].mode = gpu_exclusive_process;
        if (gpu_has_job(np, gpuidx))
          {
          np->nd_gpusn[gpuidx].state = gpu_exclusive;
          }
        else
          {
          np->nd_gpusn[gpuidx].inuse = 0;
          np->nd_gpusn[gpuidx].state = gpu_unallocated;
          }
        }
      else if (!mode.compare("Prohibited"))
        {
        np->nd_gpusn[gpuidx].mode = gpu_prohibited;
        np->nd_gpusn[gpuidx].state = gpu_unavailable;
        }
      else
        {
        /* unknown mode, default to prohibited */
        np->nd_gpusn[gpuidx].mode = gpu_prohibited;
        np->nd_gpusn[gpuidx].state = gpu_unavailable;
        if (LOGLEVEL >= 3)
          {
          sprintf(log_buffer,
            "GPU %s has unknown mode on node %s",
            gpuid.c_str(),
            np->nd_name);

          log_ext(-1, __func__, log_buffer, LOG_DEBUG);
          }
        }

      /* add gpu_mode so it gets added to the pbs_attribute */
      gpuinfo_stream << ";gpu_state=";
      switch (np->nd_gpusn[gpuidx].state)
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

    if (decode_arst(&temp, NULL, NULL, gpuinfo_stream.str().c_str(), 0))
      {
      DBPRT(("is_gpustat_get: cannot add attributes\n"));

      free_arst(&temp);

      return(DIS_NOCOMMIT);
      }

    /* reset the gpuinfo stream for the next iteration */
    gpuinfo_stream.str("");
    gpuinfo_stream.clear();
    
    /* mark that this gpu node is not virtual */
    gpus_real = true;
    } /* END */

  if (gpus_real)
    {
    /* mark that this gpu node is not virtual */
    np->nd_gpus_real = TRUE;
    }

  /* maintain the gpu count, if it has changed we need to update the nodes file */
  if (gpus_status.size() != startgpucnt)
    {
    np->nd_ngpus = gpus_status.size();

    /* update the nodes file */
    update_nodes_file(np);
    }

  node_gpustatus_list(&temp, np, ATR_ACTION_ALTER);

  return(DIS_SUCCESS);
  }  /* END pbs_read_json_gpu_status() */



/**
 * Read MIC status from JsonCPP object into the given node attributes.
 * @param np the pointer to PBS node to be updated.
 * @param mics_status the Json value object reference containing MIC status.
 * @return DIS_SUCCESS (0) if succeeded or non-zero value otherwise.
 */
int pbs_read_json_mic_status(struct pbsnode *np, const Json::Value &mics_status)
  {
  /* TODO: Implement. See process_mic_status() */
  pbs_attribute     temp;
  std::stringstream micinfo_stream;
  int               mic_count = 0;
  int               rc = PBSE_NONE;

  if (LOGLEVEL >= 7)
    {
    sprintf(log_buffer, "received mic status from node %s", (np != NULL) ? np->nd_name : "NULL");
    log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buffer);
    }

  if (mics_status.isNull() || !mics_status.isArray())
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

  for (unsigned int i = 0; i < mics_status.size(); i++)
    {
    const Json::Value &mic_status = mics_status[i];
    const Json::Value *temp_value;
    std::string micid;

    /* add the info to the "temp" attribute */

    /* get micid */
    temp_value = &mic_status["micid"];
    if (temp_value->isNull() || !temp_value->isString())
      {
      if (LOGLEVEL >= 3)
        {
        sprintf(log_buffer,
            "Failed to get/create entry for mic without micid specified on node %s\n",
            np->nd_name);

        log_ext(-1, __func__, log_buffer, LOG_DEBUG);
        }

      free_arst(&temp);
      return(DIS_NOCOMMIT);
      }
    micid = temp_value->asString();

    micinfo_stream << "mic[" << mic_count << "]=" << micid;

    // Print all other mic status values
    for (unsigned int i = 0; i < mic_status.getMemberNames().size(); i++)
      {
      std::string key = mic_status.getMemberNames()[i];
      /* micid already handled, skip it */
      if (!key.compare("micid"))
        {
        continue;
        }
      temp_value = &mic_status[key];
      if (temp_value->isString() || temp_value->isBool())
        {
        micinfo_stream << ';' << key << '=' << temp_value->asString();
        }
      else if (temp_value->isInt())
        {
        micinfo_stream << ';' << key << '=' << temp_value->asInt();
        }
      }

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

  if (mic_count > np->nd_nmics)
    {
    np->nd_nmics_free += mic_count - np->nd_nmics;
    np->nd_nmics = mic_count;

    if (mic_count > np->nd_nmics_alloced)
      {
      struct jobinfo *tmp = (struct jobinfo *)calloc(mic_count, sizeof(struct jobinfo));

      if (tmp == NULL)
        return(ENOMEM);

      memcpy(tmp, np->nd_micjobs, sizeof(struct jobinfo) * np->nd_nmics_alloced);
      free(np->nd_micjobs);
      np->nd_micjobs = tmp;

      np->nd_nmics_alloced = mic_count;
      }
    }

  node_micstatus_list(&temp, np, ATR_ACTION_ALTER);

  return(rc);
  }



/**
 * Parse the given character buffer as Json MOMs status report and updates corresponding nodes.
 * @param sz character buffer size.
 * @param data character buffer with Json data.
 * @return DIS_SUCCESS (0) if succeeded,
 *         SEND_HELLO if a node requested hello message from the server or
 *         -1 otherwise.
 */
int pbs_read_json_status(const size_t sz, const char *data)
  {
  long              mom_job_sync = 0;
  long              auto_np = 0;
  long              down_on_error = 0;
  Json::Value       root;
  Json::Reader      reader;
  std::stringstream status_stream;
  const Json::Value *temp_value;

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
  bool parsingSuccessful = reader.parse(data, data + sz - 1, root, false);
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

  const Json::Value &body = root["body"];

  if (body.isNull() || !body.isArray())
    {
    // Body have to be an array of nodes statuses
    log_err(-1, __func__, "status message containg no body");
    return -1;
    }

  const Json::Value &senderID = root[JSON_SENDER_ID_KEY];
  if (!senderID.isString())
    {
    return -1;
    }
  sprintf(log_buffer, "Got json statuses message from senderId:%s", senderID.asString().c_str());
  log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);

  get_svr_attr_l(SRV_ATR_MomJobSync, &mom_job_sync);
  get_svr_attr_l(SRV_ATR_AutoNodeNP, &auto_np);
  get_svr_attr_l(SRV_ATR_DownOnError, &down_on_error);

  for (unsigned int i = 0; i < body.size(); i++)
    {
    const Json::Value &node_status = body[i];
    pbs_attribute temp;
    bool dont_change_state = false;

    
    temp_value = &node_status["node"];
    if (temp_value->isNull() || !temp_value->isString())
      {
      log_err(-1, __func__, "received a status without node id specified. Ignored");
      continue;
      }
    std::string nodeId = temp_value->asString();

    sprintf(log_buffer, "Got status nodeId:%s", nodeId.c_str());
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);

    struct pbsnode *current = find_nodebyname(nodeId.c_str());
    if(current == NULL)
      {
      // TODO: Check is the node trusted and create an entinty for the node if not found.
      //       See svr_is_request() code.
      sprintf(log_buffer, "the node with id '%s' not found. Ignored", nodeId.c_str());
      log_err(-1, __func__, log_buffer);
      continue;
      }

    mutex_mgr node_mutex(current->nd_mutex, true);

    if (LOGLEVEL >= 10)
      {
      snprintf(log_buffer, LOCAL_LOG_BUF_SIZE, "handle status for node '%s'", nodeId.c_str());
      log_event(PBSEVENT_ADMIN,PBS_EVENTCLASS_SERVER,__func__,log_buffer);
      }

    pbs_read_json_gpu_status(current, node_status[GPU_STATUS_KEY]);
    pbs_read_json_mic_status(current, node_status[MIC_STATUS_KEY]);

    temp_value = &node_status["first_update"];
    if (!temp_value->isNull() && temp_value->isBool())
      {
      if (LOGLEVEL >= 10)
        {
        log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__,
            ("status: first_update = " + temp_value->asString()).c_str());
        }

      if (temp_value->asBool())
        {
        /* mom is requesting that we send the mom hierarchy file to her */
        remove_hello(&hellos, current->nd_name);      

        /* send hierarchy to mom*/ 
        struct hello_info *hi = (struct hello_info *)calloc(1, sizeof(struct hello_info));

        hi->name = strdup(current->nd_name);
        enqueue_threadpool_request(send_hierarchy_threadtask, hi);        

        /* reset gpu data in case mom reconnects with changed gpus */
        clear_nvidia_gpus(current);
        }
      }

    if (current->nd_mom_reported_down)
      {
      dont_change_state = true;
      }

    temp_value = &node_status["message"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      if (!temp_value->asString().compare("ERROR") && down_on_error)
        {
        update_node_state(current, INUSE_DOWN);
        dont_change_state = true;
        }
      status_stream << "message=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["state"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      if (!dont_change_state)
        {
        process_state_str_val(current, temp_value->asCString());
        }
      }

    temp_value = &node_status["uname"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      if (allow_any_mom)
        {
        process_uname_str(current, temp_value->asCString());
        }
      status_stream << "uname=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["jobdata"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      if (mom_job_sync)
        {
        update_job_data(current, temp_value->asCString());
        }
      status_stream << "jobdata=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["jobs"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      std::string value = temp_value->asString();
      if (mom_job_sync)
        {
        size_t len = value.length() + strlen(current->nd_name) + 2;
        char *jobstr = (char *)calloc(1, len);
        sync_job_info *sji = (sync_job_info *)calloc(1, sizeof(sync_job_info));

        if (jobstr == NULL || sji == NULL)
          {
          if (jobstr)
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
          snprintf(jobstr, len, "%s:%s", current->nd_name, value.c_str());
          sji->input = jobstr;
          sji->timestamp = time(NULL);

          /* sji must be freed in sync_node_jobs */
          enqueue_threadpool_request(sync_node_jobs, sji);
          }
        }
      status_stream << "jobs=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["ncpus"];
    if (!temp_value->isNull() && temp_value->isString())
      {
      handle_auto_np_val(current, temp_value->asCString());
      status_stream << "ncpus=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["availmem"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "availmem=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["idletime"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "idletime=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["netload"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "netload=" << temp_value->asString() << ";";
      }
    
    temp_value = &node_status["nsessions"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "nsessions=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["nusers"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "nusers=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["totmem"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "totmem=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["physmem"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "physmem=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["loadave"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "loadave=" << temp_value->asString() << ";";
      }
    
    temp_value = &node_status["opsys"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "opsys=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["gres"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "gres=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["varattr"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "varattr=" << temp_value->asString() << ";";
      }

    temp_value = &node_status["sessions"];
    if(!temp_value->isNull() && temp_value->isString())
      {
      status_stream << "sessions=" << temp_value->asString() << ";";
      }

    if (status_stream.rdbuf()->in_avail())
      {
      memset(&temp, 0, sizeof(temp));
      if (decode_arst(&temp, NULL, NULL, NULL, 0) != PBSE_NONE)
        {
        log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, "cannot initialize attribute");
        }
      else
        {
        // Put collected values into the temp attribute
        if (decode_arst(&temp, NULL, NULL, status_stream.str().c_str(), 0))
          {
          DBPRT(("mom_read_json_status: cannot add attributes\n"));
          free_arst(&temp);
          }
        else
          {
          save_node_status(current, &temp);
          }
        }

      /* reset the status_stream for the next iteration */
      status_stream.str("");
      status_stream.clear();
      }

    node_mutex.unlock();
    }

  return(DIS_SUCCESS);
  } /* END mom_read_json_status() */

#endif /* ZMQ */
