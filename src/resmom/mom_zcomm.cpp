#include <pbs_config.h>

#ifdef ZMQ

int read_im_request(const size_t sz, const char *data)
  {
  Json::Reader      reader;
  Json::Value       root;
  Json::Value      *temp_value;
  int               rc = 0;

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
  bool parsing_successful = reader.parse(data, data + size, root, false);
  if ( !parsing_successful )
    {
    // Can't read the message. Malformed?
    log_err(-1, __func__, "malformed Json message received");
    return -1;
    }

  temp_value = &root["messageType"];
  if (temp_value->isNull() || !temp_value->isString() || temp_value->asString() != "momRequest")
    {
    // There is no 'messageType' key in the message
    log_err(-1, __func__, "non-job control message received");
    return -1;
    }

  temp_value = &root["command"];
  if (temp_value->isNull() || !temp_value->isString())
    {
    // There is no 'command' key in the message
    log_err(-1, __func__, "no command in the message or the command isn't a string value");
    return -1;
    }
  std::string cmd = temp_value->asString();

  std::string job_id;
  if (root["jobId"].isString())
    {
    job_id = root["jobId"].asString();
    }

  std::string task_id;
  if (root["taskId"].isString())
    {
    task_id = root["taskId"].asString();
    }

  std::string cookie;
  if (root["cookie"].isString())
    {
    cookie = root["cookie"].asString();
    }

  std::string event;
  if (root["event"].isString())
    {
    event = root["event"].asString();
    }

  if (cmd == "joinJob")
    {
      return process_jj_request(job_id, cookie, event, task_id, root["body"]);
    }
  else
    {
    // There is no 'command' key in the message
    snprintf(log_buffer, LOG_BUF_SIZE, "Unknown command '%s' passed", cmd.c_str());
    log_err(-1, __func__, log_buffer);
    return -1;
    }
  }



int process_jj_request(const std::string &job_id, const std::string &cookie,
    const std::string &event, const std::string &task_id, const std::string &sender_id,
    const JSon::Value &body)
  {
  job *pjob;
  int  node_id;
  int  node_num;
  int  radix = 0;
  std::vector<std::string> radix_hosts;
  std::vector<int> radix_ports;

  if (!body.isObject())
    {
    return -1;
    }

  if (!body["nodeId"].isInt())
    {
    sprintf(log_buffer,"join_job request for job %s failed - nodeId is absent or isn't integer type",
        job_id);
    log_err(-1, __func__, log_buffer);
    return(IM_FAILURE);
    }
  node_id = body["node_id"].asInt();

  if (!body["nodeNum"].isInt())
    {
    sprintf(log_buffer, "join_job request from node %d for job %s failed - nodeNum is absent or "
        "isn't integer type", node_id, job_id);
    log_err(-1, __func__, log_buffer);
    return(IM_FAILURE);
    }
  node_num = body["nodeNum"].asInt();

  if (body["radix"].isInt())
    {
    radix = body["radix"].asInt();
    }

  // Does job already exist?
  ret = get_job_struct_json(&pjob, job_id, node_id, sender_id, body);

  if (ret != PBSE_NONE)
    {
    if (ret == PBSE_DISPROTO)
      {
      return(IM_FAILURE);
      }
    else 
      {
      return(IM_DONE);
      }
    }
  
  pjob->ji_numnodes = node_num;


  /* insert block based on radix */
  if (radix > 0)
    {
    /* Get the nodes for this radix */
    for (const Json::ValueIterator itr = body["nodesArray"].begin(); itr != body["nodesArray"].end(); itr++)
      {
      const Json::Value &host = (*itr)["nodeId"];
      const Json::VAlue &port = (*itr)["port"];
      if (!host.isString() || !port.isInt())
        {
        sprintf(log_buffer, "%s: join_job_radix request to node %d for job %s failed - wrong radix "
            "nodes array", __func__, node_id, job_id.c_str());
        log_err(-1, __func__, log_buffer);
        return(IM_FAILURE);
        }
      radix_hosts.push_back(host.asString());
      radix_ports.push_back(port.asInt());
      }
    } /* END if radix > 0 */

  CLEAR_HEAD(lhead);

  if (read_attributes(body["attrArray"], &lhead) != PBSE_NONE)
    {
    sprintf(log_buffer, "%s: join_job request to node %d for job %s failed - wrong attribute list",
      __func__, nodeid, jobid);
    log_err(-1, __func__, log_buffer);
   
    return(IM_FAILURE);
    }
  
  /* Get the hashname from the pbs_attribute. */
  psatl = (svrattrl *)GET_NEXT(lhead);
  
  while (psatl)
    {
    if (!strcmp(psatl->al_name, ATTR_hashname))
      {
      snprintf(basename, sizeof(basename), "%s", psatl->al_value);
      
      break;
      }
    
    psatl = (svrattrl *)GET_NEXT(psatl->al_link);
    }

  snprintf(pjob->ji_qs.ji_jobid, sizeof(pjob->ji_qs.ji_jobid), "%s", jobid);
  snprintf(pjob->ji_qs.ji_fileprefix, sizeof(pjob->ji_qs.ji_fileprefix), "%s", basename);
  
  pjob->ji_modified       = 1;
  pjob->ji_nodeid         = nodeid;
  
  pjob->ji_qs.ji_svrflags = 0;
  if (radix > 0)
    {
    pjob->ji_qs.ji_svrflags |= JOB_SVFLG_INTERMEDIATE_MOM;
    }

  pjob->ji_qs.ji_un_type  = JOB_UNION_TYPE_MOM;
  
  /* decode attributes from request into job structure */
  
  rc = 0;
  resc_access_perm = READ_WRITE;
  
  for (psatl = (svrattrl *)GET_NEXT(lhead); psatl; psatl = (svrattrl *)GET_NEXT(psatl->al_link))
    {
    /* identify the pbs_attribute by name */
    
    index = find_attr(job_attr_def, psatl->al_name, JOB_ATR_LAST);
    
    if (index < 0)
      {
      /* didn`t recognize the name */
      
      rc = PBSE_NOATTR;
      
      break;
      }

    pdef = &job_attr_def[index];
    
    /* decode pbs_attribute */
    
    if ((rc = pdef->at_decode(&pjob->ji_wattr[index],
          psatl->al_name, psatl->al_resc, psatl->al_value, resc_access_perm)) != PBSE_NONE)
      break;
    }  /* END for (psatl) */
  
  free_attrlist(&lhead);
  
  if (rc != 0)
    {
    if (LOGLEVEL >= 6)
      {
      sprintf(log_buffer, "%s:error %d received in joinjob - purging job",
        __func__, 
        rc);
      
      log_event(PBSEVENT_JOB,PBS_EVENTCLASS_JOB,pjob->ji_qs.ji_jobid,log_buffer);
      }
    
    // TODO: Implement error send
    send_im_error(rc, pjob, cookie, event, "joinJob");
   
    mom_job_purge(pjob);

    return(IM_DONE);
    }
  
  job_nodes(pjob);
  
  /* set remaining job structure elements */
  
  pjob->ji_qs.ji_state    = JOB_STATE_TRANSIT;
  pjob->ji_qs.ji_substate = JOB_SUBSTATE_PRERUN;
  pjob->ji_qs.ji_stime    = time_now;
  pjob->ji_wattr[JOB_ATR_mtime].at_val.at_long = (long)time_now;
  
  pjob->ji_wattr[JOB_ATR_mtime].at_flags |= ATR_VFLAG_SET;
  
  /* check_pwd is setting up ji_un as type MOM
   * pjob->ji_qs.ji_un_type = JOB_UNION_TYPE_NEW;
   * pjob->ji_qs.ji_un.ji_newt.ji_fromsock = -1;
   * pjob->ji_qs.ji_un.ji_newt.ji_fromaddr = addr->sin_addr.s_addr;
   * pjob->ji_qs.ji_un.ji_newt.ji_scriptsz = 0;
   **/

  if (check_pwd(pjob) == NULL)
    {
    /* log_buffer populated in check_pwd() */
    
    log_event(PBSEVENT_JOB, PBS_EVENTCLASS_JOB, pjob->ji_qs.ji_jobid, log_buffer);
    
    send_im_error(PBSE_BADUSER, pjob, cookie, event, "joinJob");
    
    mom_job_purge(pjob);

    return(IM_DONE);
    }

  /* should we make a tmpdir? */
  if (TTmpDirName(pjob, namebuf, sizeof(namebuf)))
    {
    if (TMakeTmpDir(pjob, namebuf) != PBSE_NONE)
      {
      log_event(
        PBSEVENT_JOB,
        PBS_EVENTCLASS_JOB,
        pjob->ji_qs.ji_jobid,
        "cannot create tmp dir");
      
      send_im_error(PBSE_BADUSER, pjob, cookie, event, "joinJob");
      
      mom_job_purge(pjob);
      
      return(IM_DONE);
      }
    }
  
#ifdef PENABLE_LINUX26_CPUSETS
#ifndef NUMA_SUPPORT
  
  if (use_cpusets(pjob) == TRUE)
    {
    sprintf(log_buffer, "about to create cpuset for job %s.\n",
      pjob->ji_qs.ji_jobid);
    
    log_ext(-1, __func__, log_buffer, LOG_INFO);

    if (create_job_cpuset(pjob) == FAILURE)
      {
      sprintf(log_buffer, "Could not create cpuset for job %s.\n",
        pjob->ji_qs.ji_jobid);

      log_err(-1, __func__, log_buffer);
      }
    }
  
#endif  /* ndef NUMA_SUPPORT */
#endif  /* (PENABLE_LINUX26_CPUSETS) */
    
  ret = run_prologue_scripts(pjob);
  if (ret != PBSE_NONE)
    {
    send_im_error(ret, pjob, cookie, event, "joinJob");
    
    mom_job_purge(pjob);
    
    return(IM_DONE);
    }
  
#if IBM_SP2==2  /* IBM SP with PSSP 3.1 */
  
  if (load_sp_switch(pjob) != 0)
    {
    send_im_error(PBSE_SYSTEM, pjob, cookie, event, "joinJob");
    
    log_err(-1, __func__, "cannot load sp switch table");
    
    mom_job_purge(pjob);
    
    return(IM_DONE);
    }
  
#endif /* IBM SP */
  
  if (multi_mom)
    {
    momport = pbs_rm_port;
    }
  
  job_save(pjob, SAVEJOB_FULL, momport);

  sprintf(log_buffer, "JOIN JOB as node %d",
    nodeid);
  
  log_record(PBSEVENT_JOB,PBS_EVENTCLASS_JOB,jobid,log_buffer);

  if ((job_radix == TRUE) &&
      (radix_hosts.size() > 2))
    {
    /* handle the case where we're contacting multiple nodes */
    if ((pjob->ji_wattr[JOB_ATR_job_radix].at_flags & ATR_VFLAG_SET) &&
        (pjob->ji_wattr[JOB_ATR_job_radix].at_val.at_long != 0))
      {
      pjob->ji_radix = pjob->ji_wattr[JOB_ATR_job_radix].at_val.at_long;
      }

    pjob->ji_im_nodeid = 1; /* this will identify us as an intermediate node later */

    if (allocate_demux_sockets(pjob, INTERMEDIATE_MOM))
      {
      return(IM_DONE);
      }

    contact_sisters_zmq(pjob,event,radix_hosts,radix_ports);
    pjob->ji_intermediate_join_event = event;
    job_save(pjob,SAVEJOB_FULL,momport);

    return(IM_DONE);
    }
  else
    {
    unsigned short  af_family;
    char           *host_addr = NULL;
    int             addr_len;
    int             local_errno;

    /* handle the single contact case */
    if (job_radix == TRUE)
      {
      sister_job_nodes_zmq(pjob, radix_hosts, radix_ports);

      np = &pjob->ji_sisters[0];
      if (np != NULL)
        {
        ret = get_hostaddr_hostent_af(&local_errno, np->hn_host, &af_family, &host_addr, &addr_len);
        memmove(&np->sock_addr.sin_addr, host_addr, addr_len);
        free(host_addr);
        np->sock_addr.sin_port = htons(np->hn_port);
        np->sock_addr.sin_family = af_family;
        }

      /* This is a leaf node in the job radix hierarchy. pjob->ji_radix needs to be set to non-zero
         for later in tm_spawn calls. */
      pjob->ji_radix = 2;
      }
    }
    
  /*
   ** if certain resource limits require that the job usage be
   ** polled, we link the job to mom_polljobs.
   **
   ** NOTE: we overload the job field ji_jobque for this as it
   ** is not used otherwise by MOM
   */
  if (mom_do_poll(pjob))
    append_link(&mom_polljobs, &pjob->ji_jobque, pjob);
  
  append_link(&svr_alljobs, &pjob->ji_alljobs, pjob);
  
  /* establish a connection and write the reply back */
  if ((reply_to_join_job_as_sister(pjob, addr, cookie, event, fromtask, job_radix)) == DIS_SUCCESS)
    ret = IM_DONE;

  return(ret);
  } /* END mom_read_json_status() */



/* Get the job info. If the job exists get it. If not make a new one */
int get_job_struct_json(job **pjob, const std::string &job_id, const std::string &node_id,
    const std::string &sender_id, const Json::Value &body)
  {
  int  ret;
  job *new_job;

  new_job = mom_find_job(job_id.c_str());

  if (new_job != NULL)
    {
    /* job already exists locally */

    if (new_job->ji_qs.ji_substate == JOB_SUBSTATE_PRERUN)
      {
      if (LOGLEVEL >= 3)
        {
        /* if peer mom times out, MS will send new join request for same job */
        sprintf(log_buffer,
            "WARNING: duplicate JOIN request joinJob from %s (purging previous pjob)",
            sender_id.c_str());
        log_event( PBSEVENT_JOB, PBS_EVENTCLASS_JOB, jobid, log_buffer);
        }
      mom_job_purge(new_job);
      }
    else
      {
      if (LOGLEVEL >= 0)
        {
        sprintf(log_buffer, "ERROR: received request 'joinJob' from %s (job already exists locally)",
                sender_id.c_str());
        log_event(PBSEVENT_JOB, PBS_EVENTCLASS_JOB, jobid, log_buffer);
        }

      /* should local job be purged, ie 'mom_job_purge(pjob);' ? */
      return PBSE_JOBEXIST;
      }
    }  /* END if (pjob != NULL) */

  if ((new_job = job_alloc()) == NULL)
    {
    /* out of memory */
    log_err(-1, __func__, "insufficient memory to create job");
    return PBSE_SYSTEM;
    }

  if (!body["stdoutPort"].isInt())
    {
    sprintf(log_buffer, "%s: join_job request to node %s for job %s failed - stdoutPort isn't "
        "specified or non-integer type", __func__, node_id, job_id);
    log_err(-1, __func__, log_buffer);
    return PBSE_DISPROTO;
    }
  new_job->ji_portout = body["stdoutPort"].asInt();

  if (!body["stderrPort"].isInt())
    {
    sprintf(log_buffer, "%s: join_job request to node %d for job %s failed - stderrPort isn't "
        "specified or non-integer type", __func__, node_id, job_id);
    log_err(-1, __func__, log_buffer);

    return PBSE_DISPROTO;
    }
  new_job->ji_porterr = body["stderrPort"].asInt();

  *pjob = new_job;
  return PBSE_NONE;
  } /* END get_job_struct_json() */



/* create the list of sisters to contact for this radix group
 * and send the job along
 */
int contact_sisters_zmq(
  job        *pjob,
  tm_event_t  event,
  const std::vector &radix_hosts,
  const std::vector &radix_ports)

  {
  int                index;
  int                j;
  int                i;
  int                mom_radix;
  hnodent           *np;
  struct radix_buf **sister_list;
  int                ret;
  tlist_head         phead;
  pbs_attribute     *pattr;
  
  char              *host_addr = NULL;
  int                addr_len;
  int                local_errno;
  unsigned short     af_family;

  /* we have to have a sister count of 2 or more for
     this to work */
  if (radix_hosts.size() <= 2)
    {
    return(-1);
    }

  mom_radix = pjob->ji_radix;

  CLEAR_HEAD(phead);

  pattr = pjob->ji_wattr;

  /* prepare the attributes to go out on the wire. at_encode does this */
  for (i = 0;i < JOB_ATR_LAST;i++)
    {
    (job_attr_def + i)->at_encode(
  	  pattr + i,
  	  &phead,
  	  (job_attr_def + i)->at_name,
  	  NULL,
  	  ATR_ENCODE_MOM,
      ATR_DFLAG_ACCESS);
    }  /* END for (i) */

  attrl_fixlink(&phead);

  /* NYI: this code performs unnecessary steps. Fix later */

  /* We have to put this job into the proper queues. These queues are filled
	 in req_quejob and req_commit on Mother Superior for non-job_radix jobs */
  append_link(&svr_newjobs, &pjob->ji_alljobs, pjob); /* from req_quejob */

  delete_link(&pjob->ji_alljobs); /* from req_commit */
  append_link(&svr_alljobs, &pjob->ji_alljobs, pjob); /* from req_commit */

  /* initialize the nodes for every sister in this job
     only the first mom_radix+1 entries will be used
     for communication */
  sister_job_nodes_zmq(pjob, radix_hosts, radix_ports);

  /* we now need to create the list of sisters to send to
     our intermediate MOMs in our job_radix */
  sister_list = allocate_sister_list(mom_radix+1);

  /* We need to get the address and port of the MOM who
     called us (pjob->ji_sister[0]) so we can contact
     her when we call back later */
  np = &pjob->ji_sisters[0];
  ret = get_hostaddr_hostent_af(&local_errno, np->hn_host, &af_family, &host_addr, &addr_len);
  memmove(&np->sock_addr.sin_addr, host_addr, addr_len);
  np->sock_addr.sin_port = htons(np->hn_port);
  np->sock_addr.sin_family = af_family;

  /* Set this MOM as the first entry for everyone in the
     job_radix. This is how the children will know who
     called them. */
  index = 1;
  for (j = 0; j <= mom_radix && j < sister_count-1; j++)
    {
    np = &pjob->ji_sisters[index];
    add_host_to_sister_list(np->hn_host, np->hn_port, sister_list[j]);
    ret = get_hostaddr_hostent_af(&local_errno, np->hn_host, &af_family, &host_addr, &addr_len);
    memmove(&np->sock_addr.sin_addr, host_addr, addr_len);
    np->sock_addr.sin_port = htons(np->hn_port);
    np->sock_addr.sin_family = af_family;
    index++;
    }

  free_sisterlist(sister_list, mom_radix+1);

  sister_list = allocate_sister_list(mom_radix);

  /* Add this node as the first node in each sister_list */
  np = &pjob->ji_sisters[1];
  for (i = 0; i < mom_radix; i++)
    {
    add_host_to_sister_list(np->hn_host, np->hn_port, sister_list[i]);
    }

  index = 2;   /* index 2 is the first child node. */

  do
    {
    for (j = 0; j < mom_radix && index < sister_count; j++)
      {
      /* Generate a list of sisters divided in to 'mom_radix' number of lists.
         For example an exec_host list of host1+host2+host3+host4+host5+host6+host7
         would create sister lists on a mom_radix of 3 like the following
         host1+host4+host7
         host2+host5
         host3+host6
      */
      np = &pjob->ji_sisters[index];
      add_host_to_sister_list(np->hn_host, np->hn_port, sister_list[j]);
      index++;
      }
    } while (index < sister_count);

  pjob->ji_sisters[1].hn_node = 1; /* This will also identify us an an intermediate node later */

  /* we go to pjob->ji_sisters[1] because we do not want to include the parent node that
   	 sent the IM_JOIN_JOB_RADIX request as a sister to lower MOMs */
  ret = open_tcp_stream_to_sisters(pjob,
      IM_JOIN_JOB_RADIX,
      event,
      mom_radix,
      &pjob->ji_sisters[1],
      sister_list,
      &phead,
      INTERMEDIATE_MOM);

  free_sisterlist(sister_list, mom_radix);
  free_attrlist(&phead);

  return(ret);
  } /* END contact_sisters */





int read_attributes(const Json::Value &attrs, tlist_head *phead)
  {
  unsigned int  i;
  unsigned int  hasresc;
  size_t        ls;
  unsigned int  data_len;
  svrattrl     *psvrat = NULL;
  int           rc;
  size_t        tsize;

  if (!attrs.isArray())
    {
    return(-1);
    }

  for (const Json::ValueIterator itr = attrs.begin(); itr != attrs.end(); itr++)
    {
    std::string attr_name;
    std::string attr_resource;
    std::string attr_value;
    unsigned int attr_op;
    unsigned int attr_flags;

    const Json::Value *temp_val;

    temp_val = &((*itr)["name"]);
    if (!temp_val->isString())
      {
      rc = -1;
      break;
      }
    attr_name = temp_val->asString();

    temp_val = &((*itr)["value"]);
    if (!temp_val->isString())
      {
      rc = -1;
      break;
      }
    attr_value = temp_val->asString();

    temp_val = &((*itr)["resource"]);
    if (temp_val->isString())
      {
      attr_resource = temp_val->asString();
      }

    temp_val = &((*itr)["batchOp"]);
    if (!temp_val->isInt())
      {
      rc = -1;
      break;
      }
    attr_op = (unsigned int)temp_val->asInt();

    temp_val = &((*itr)["flags"]);
    if (!temp_val->isInt())
      {
      rc = -1;
      break;
      }
    attr_flags = (unsigned int)temp_val->asInt();

    data_len = attr_name.size() + 1 + attr_value.size() + 1;
    if (!attr_resource.empty())
      {
      data_len += attr_resource.size() + 1;
      }

    tsize = sizeof(svrattrl) + data_len;

    if ((psvrat = (svrattrl *)calloc(1, tsize)) == 0)
      return (PBSE_MEM_MALLOC);

    CLEAR_LINK(psvrat->al_link);

    psvrat->al_atopl.next = 0;
    psvrat->al_tsize = tsize;
    psvrat->al_flags = 0;
    psvrat->al_op = (enum batch_op)attr_op;

    psvrat->al_name = (char *)psvrat + sizeof(svrattrl);
    psvrat->al_nameln = attr_name.size() + 1;
    std::copy(attr_name.begin(), attr_name.end(), psvrat->al_name);
    psvrat->al_name[attr_name.size()] = "\0";

    if (!attr_resource.empty())
      {
      psvrat->al_resc = psvrat->al_name + psvrat->al_nameln;
      psvrat->al_rescln = attr_resource.size() + 1;
      std::copy(attr_resource.begin(), attr_resource.end(), psvrat->al_resc);
      psvrat->al_resc[attr_resource.size()] = "\0";
      }

    psvrat->al_value = psvrat->al_resc + psvrat->al_rescln;
    psvrat->al_valln = attr_value.size() + 1;
    std::copy(attr_value.begin(), attr_value.end(), psvrat->al_value);
    psvrat->al_value[attr_value.size()] = "\0";

    append_link(phead, &psvrat->al_link, psvrat);
    }

  if (rc)
    {
    (void)free(psvrat);
    }

  return (rc);
  }



/**
 * This is similar to job_nodes only this is done for
 * intermediate moms when a job_radix has been set.
 * The intermediate moms need to know what sisters they
 * are in charge of. sister_job_nodes parses the list of
 * sisters given to the intermediate mom from its parent
 * and adds the names to its copy of the job structure.
 * To clarify, intermediate moms will have a slightly different
 * copy of a job than the rest of the sisters because they
 * will keep track of the sisters in their radix.
 *
 * @see start_exec() - parent
 */

void sister_job_nodes_zmq(

  job *pjob,
  const std::vector<string> &radix_hosts,
  const std::vector<string> &radix_ports )  /* I */

  {
  int      i;
  int      j;
  int      nhosts;
  int      ix;
  char    *cp = NULL;
  char    *nodestr = NULL;
  char    *portstr = NULL;
  hnodent *hp = NULL;
  vnodent *np = NULL;

  /*  nodes_free(pjob); We may need to do a sister_nodes_free later */

  if (radix_hosts.size() != radix_ports.size())
    {
    return;
    }

  pjob->ji_sisters = (hnodent *)calloc(radix_hosts.size() + 1, sizeof(hnodent));
  if (pjob->ji_sisters == NULL)
    return;

  pjob->ji_sister_vnods = (vnodent *)calloc(radix_hosts.size() + 1, sizeof(vnodent));
  if (pjob->ji_sister_vnods == NULL)
    return;

  nhosts = 0;

  np = pjob->ji_sister_vnods;

  for (i = 0; i < radix_hosts.size(); i++, np++)
    {
    std::string nodename;
    size_t tmp_idx;

    ix = 0;
    tmp_idx = radix_hosts[i].find('/');
    nodename = radix_hosts[i].substr(0, tmp_idx);
    if (tmp_idx != std::string::npos)
      {
      std::stringstream convert(radix_hosts[i].substr(tmp_idx + 1));
      if (!(convert >> ix))
        {
        ix = 0;
        }
      }

    /* see if we already have this host */

    for (j = 0;j < nhosts;++j)
      {
      if (nodename.compare(pjob->ji_sisters[j].hn_host) == 0)
        break;
      }

    hp = &pjob->ji_sisters[j];

    if (j == nhosts)
      {
      /* need to add host to tn_host */

      hp->hn_node = nhosts++;
      hp->hn_sister = SISTER_OKAY;
      hp->hn_host = strdup(nodename.c_str());
      hp->hn_port = radix_ports[i];

      CLEAR_HEAD(hp->hn_events);
      }

    np->vn_node  = i;  /* make up node id */

    np->vn_host  = &pjob->ji_sisters[j];
    np->vn_index = ix;

    if (LOGLEVEL >= 4)
      {
      sprintf(log_buffer, "%d: %s/%d",
        np->vn_node,
        np->vn_host->hn_host,
        np->vn_index);
      
      log_record(PBSEVENT_ERROR, PBS_EVENTCLASS_JOB, __func__, log_buffer);
      }
    }   /* END for (i) */

  np->vn_node = TM_ERROR_NODE;

  pjob->ji_sisters[nhosts].hn_node = TM_ERROR_NODE;

  pjob->ji_numsisternodes = nhosts;

  pjob->ji_numsistervnod  = nodenum;

  if (LOGLEVEL >= 2)
    {
    sprintf(log_buffer, "job: %s numnodes=%d numvnod=%d",
      pjob->ji_qs.ji_jobid,
      nhosts,
      nodenum);

    log_record(PBSEVENT_ERROR, PBS_EVENTCLASS_JOB, __func__, log_buffer);
    }

  return;
  }   /* END sister_job_nodes_zmq() */




void send_im_error(
    int         err,
    job        *pjob,
    char       *cookie,
    tm_event_t  event,
    tm_task_id  fromtask,
    const std::string &command)
  {
  int              socket;
  int              i;
  int              rc = DIS_SUCCESS;
  struct tcp_chan *local_chan = NULL;


  TrqJson::JobControlMessage msg;
  msg.setMomId(sender_id);
  msg.setMessageType("momResponse");
  msg.setCommand(command);
  msg.setJobId(pjob->ji_qs.ji_jobid);
  msg.setCookie(cookie);
  msg.setEvent(event);
  msg.setTaskId(fromtask);
  msg.setError(err);

  send_im_msg(pjob, msg);
  }

int send_im_msg_to_parent(job *pjob, msg)
  {
  rc = -1;
  int linger = 60000;
  zmq_msg_t zmq_msg;

  /* create new socket */
  if (pjob.zsocket == NULL)
    {
    pjob.zsocket = zmq_socket(g_zmq_context, ZMQ_DEALER);
    }

  if (!pjob.zsocket)
    {
    log_err(errno, __func__, "unable to create a socket");
    }
  /* connect */
  else if (zconnect(pjob.zsocket, pjob->ji_sisters[0].sock_addr, 0) != 0)
    {
    }
  /* init message */
  else if (zinit_msg(&zmq_msg, msg) != 0)
    {
    }
  /* send message */
  else if (send_zmq_msg(zsock, msg) != 0)
    {
    rc = 0;
    }

  /* close the socket */
  if (socket)
    {
    zmq_close(socket);
    }

  return rc;
  
  } /* END send_im_error() */



#endif /* ZMQ */
