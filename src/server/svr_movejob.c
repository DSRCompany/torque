/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/
/*
 * svr_movejob.c - functions to move a job to another queue
 *
 * Included functions are:
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <semaphore.h>

#include <pbs_config.h>   /* the master config generated by configure */

#include "libpbs.h"
#include "pbs_error.h"
#include "list_link.h"
#include "attribute.h"
#include "server_limits.h"
#include "work_task.h"
#include "log.h"
#include "../lib/Liblog/pbs_log.h"
#include "../lib/Liblog/log_event.h"
#include "queue.h"
#include "pbs_job.h"
#include "pbs_nodes.h"
#include "credential.h"
#include "batch_request.h"
#include "net_connect.h"
#include "svrfunc.h"
#include "mcom.h"
#include "array.h"
#include "threadpool.h"
#include "../lib/Libutils/u_lock_ctl.h" /* unlock_node */
#include "queue_func.h" /* find_queuebyname */
#include "req_runjob.h" /* finish_sendmom */
#include "ji_mutex.h"

#if __STDC__ != 1
#include <memory.h>
#endif


/* reduced retry from 3 to 2 (CRI - Mar 23, 2004) */

#define RETRY 2 /* number of times to retry network move */

/* External functions called */

extern void remove_stagein(job **);
extern void remove_checkpoint(job **);
extern int  job_route(job *);
extern int svr_job_purge(job *);
int PBSD_commit_get_sid(int ,long *,char *);
int get_job_file_path(job *,enum job_file, char *, int);

extern struct pbsnode *PGetNodeFromAddr(pbs_net_t);



/* Private Functions local to this file */

static int  local_move(job *, int *, struct batch_request *, int);
static int should_retry_route(int err);

/* Global Data */

extern int              LOGLEVEL;
extern char             *path_jobs;
extern char             *path_spool;
extern attribute_def     job_attr_def[];
extern int               queue_rank;
extern char              server_host[];
extern char              server_name[];
extern char             *msg_badexit;
extern char             *msg_routexceed;
extern char             *msg_manager;
extern char             *msg_movejob;
extern char             *msg_err_malloc;
extern int               comp_resc_gt;
extern int               comp_resc_eq;
extern int               comp_resc_lt;
extern char             *pbs_o_host;
extern pbs_net_t         pbs_server_addr;
extern unsigned int      pbs_server_port_dis;
extern time_t            pbs_tcp_timeout;
extern int               LOGLEVEL;

int net_move(job *, struct batch_request *);

/*
 * svr_movejob
 *
 * Test if the destination is local or not and call a routine to
 * do the appropriate move.
 *
 * Returns:
 *  0 success
 *        -1 permenent failure or rejection,
 *  1 failed but try again
 *  2 deferred (ie move in progress), check later
 */

int svr_movejob(

  job                  *jobp,
  char                 *destination,
  int                  *my_err,
  struct batch_request *req,
  int                   parent_queue_mutex_held)

  {
  pbs_net_t     destaddr;
  int           local;
  unsigned int  port;
  char         *toserver;
  char          log_buf[LOCAL_LOG_BUF_SIZE];

  if (LOGLEVEL >= 7)
    {
    sprintf(log_buf, "%s", jobp->ji_qs.ji_jobid);
    LOG_EVENT(PBSEVENT_JOB, PBS_EVENTCLASS_JOB, __func__, log_buf);
    }

 if (strlen(destination) >= (size_t)PBS_MAXROUTEDEST)
    {
    sprintf(log_buf, "name %s over maximum length of %d\n",
      destination,
      PBS_MAXROUTEDEST);

    log_err(-1, "svr_movejob", log_buf);

    *my_err = PBSE_QUENBIG;

    return(-1);
    }

  snprintf(jobp->ji_qs.ji_destin, sizeof(jobp->ji_qs.ji_destin), "%s", destination);

  jobp->ji_qs.ji_un_type = JOB_UNION_TYPE_ROUTE;

  local = 1;

  if ((toserver = strchr(destination, '@')) != NULL)
    {
    /* check to see if the part after '@' is this server */
    char *tmp = parse_servername(++toserver, &port);

    destaddr = get_hostaddr(my_err, tmp);

    if (destaddr != pbs_server_addr)
      {
      local = 0;
      }

    free(tmp);
    }

  if (local != 0)
    {
    return(local_move(jobp, my_err, req, parent_queue_mutex_held));
    }

  return(net_move(jobp, req));
  }  /* svr_movejob() */




/*
 * local_move - internally move a job to another queue
 *
 * Check the destination to see if it can accept the job.
 *
 * Returns:
 *  0 success
 * -1 permanent failure or rejection
 *  1 failed but try again
 */

static int local_move(

  job                  *pjob,
  int                  *my_err,
  struct batch_request *req,
  int                   parent_queue_mutex_held)

  {
  pbs_queue *routing_que;
  pbs_queue *dest_que;
  pbs_queue *tmp_que;
  char      *destination = pjob->ji_qs.ji_destin;
  int        mtype;
  char       log_buf[LOCAL_LOG_BUF_SIZE];
  char       job_id[PBS_MAXSVRJOBID+1];
  int        rc;

  if (LOGLEVEL >= 7)
    {
    sprintf(log_buf, "%s", pjob->ji_qs.ji_jobid);
    LOG_EVENT(PBSEVENT_JOB, PBS_EVENTCLASS_JOB, __func__, log_buf);
    }
  /* search for destination queue */
  /* CAUTION!!! This code is very complex - be very careful editing */
  if (parent_queue_mutex_held == TRUE)
    routing_que = pjob->ji_qhdr;
  else
    {
    routing_que = get_jobs_queue(&pjob);

    if (pjob == NULL)
      {
      log_err(PBSE_JOBNOTFOUND, __func__, "Job lost while acquiring queue 14");
      return(PBSE_JOBNOTFOUND);
      }
    }

  if (routing_que == NULL)
    {
    sprintf(log_buf, "queue %s does not exist\n", pjob->ji_qs.ji_queue);

    log_err(-1, __func__, log_buf);

    *my_err = PBSE_UNKQUE;

    return(-1);
    }
   

  if (get_parent_dest_queues(pjob->ji_qs.ji_queue, destination, &routing_que, &dest_que, &pjob) != PBSE_NONE)
    {
    if (dest_que != NULL)
      unlock_queue(dest_que, __func__, NULL, 0);

    if ((parent_queue_mutex_held == FALSE) &&
        (routing_que != NULL))
      unlock_queue(routing_que, __func__, NULL, 0);

    if (pjob == NULL)
      return(-10);
    else
      return(-1);
    }

  /*
   * if being moved at specific request of administrator, then
   * checks on queue availability, etc. are skipped;
   * otherwise all checks are enforced.
   */

  if (req == 0)
    {
    mtype = MOVE_TYPE_Route; /* route */
    }
  else if (req->rq_perm & (ATR_DFLAG_MGRD | ATR_DFLAG_MGWR))
    {
    mtype = MOVE_TYPE_MgrMv; /* privileged move */
    }
  else
    {
    mtype = MOVE_TYPE_Move; /* non-privileged move */
    }

  /* check the destination */
  if ((*my_err = svr_chkque(
                     pjob,
                     dest_que,
                     get_variable(pjob, pbs_o_host), mtype, NULL)))
    {
    unlock_queue(dest_que, __func__, NULL, 0);

    /* should this queue be retried? */
    if (parent_queue_mutex_held == FALSE)
      unlock_queue(routing_que, __func__, "retry", LOGLEVEL);

    return(should_retry_route(*my_err));
    }

  /* dequeue job from present queue, update destination and */
  /* queue_rank for new queue and enqueue into destination  */

  strcpy(job_id, pjob->ji_qs.ji_jobid);
  unlock_ji_mutex(pjob, __func__, "1", LOGLEVEL);
  
  /* if we come out of svr_dequejob successfully pjob->ji_mutex will be locked */
  rc = svr_dequejob(job_id, TRUE); 
  if (rc)
    return(rc);

  strcpy(pjob->ji_qs.ji_queue, destination);

  pjob->ji_wattr[JOB_ATR_qrank].at_val.at_long = ++queue_rank;
    
  unlock_queue(dest_que, __func__, NULL, 0);
  unlock_queue(routing_que, __func__, "success", LOGLEVEL);

  if ((*my_err = svr_enquejob(pjob, FALSE, -1)) == PBSE_JOB_RECYCLED)
    return(-1);

  if (parent_queue_mutex_held == TRUE)
    {
    /* re-lock the routing queue */
    if ((tmp_que = lock_queue_with_job_held(routing_que, &pjob)) == NULL)
      lock_queue(routing_que, __func__, NULL, 0);
    else
      routing_que = tmp_que;
    }

  if (*my_err != PBSE_NONE)
    {
    return(-1); /* should never ever get here */
    }

  if (pjob != NULL)
    {
    pjob->ji_lastdest = 0; /* reset in case of another route */
    
    job_save(pjob, SAVEJOB_FULL, 0);
    }

  return(PBSE_NONE);
  }  /* END local_move() */




void finish_routing_processing(

  job *pjob,
  int  status)

  {
  pbs_queue   *pque;
  int          newstate;
  int          newsub;


  if (LOGLEVEL >= 10)
    LOG_EVENT(PBSEVENT_JOB, PBS_EVENTCLASS_JOB, __func__, pjob->ji_qs.ji_jobid);

  switch (status)
    {
    case LOCUTION_SUCCESS:  /* normal return, job was routed */

      if (pjob->ji_qs.ji_svrflags & JOB_SVFLG_StagedIn)
        remove_stagein(&pjob);

      if (pjob != NULL)
        {
        if (pjob->ji_qs.ji_svrflags & JOB_SVFLG_CHECKPOINT_COPIED)
          remove_checkpoint(&pjob);

        if (pjob != NULL)
          svr_job_purge(pjob); /* need to remove server job struct */
        }

      break;

    case LOCUTION_FAIL:  /* permanent rejection (or signal) */

      if (pjob->ji_qs.ji_substate == JOB_SUBSTATE_ABORT)
        {
        /* job delete in progress, just set to queued status */
        svr_setjobstate(pjob, JOB_STATE_QUEUED, JOB_SUBSTATE_ABORT, FALSE);
        
        svr_mailowner(pjob, 'a', TRUE, "Coudln't route job to remote server");

        unlock_ji_mutex(pjob, __func__, "1", LOGLEVEL);

        return;
        }

      add_dest(pjob);  /* else mark destination as bad */

      /* fall through */

    default: /* try routing again */
       
      svr_mailowner(pjob, 'a', TRUE, "Coudln't route job to remote server");

      /* force re-eval of job state out of Transit */

      svr_evaljobstate(pjob, &newstate, &newsub, 1);
      svr_setjobstate(pjob, newstate, newsub, FALSE);

      /* need to have queue's mutex when entering job_route */
      if ((pque = get_jobs_queue(&pjob)) != NULL)
        {
        if ((status = job_route(pjob)) == PBSE_ROUTEREJ)
          job_abt(&pjob, pbse_to_txt(PBSE_ROUTEREJ));
        else if (status != 0)
          job_abt(&pjob, msg_routexceed);
        else
          unlock_ji_mutex(pjob, __func__, "2", LOGLEVEL);

        unlock_queue(pque, __func__, NULL, LOGLEVEL);
        }
      else if (pjob != NULL)
        {
        /* Currently, abort if the job has no queue.
         * Should a queue be assigned in this case? */
        job_abt(&pjob, msg_routexceed);
        }

      break;
    }  /* END switch (status) */

  return;
  } /* END finish_routing_processing() */





void finish_moving_processing(

  job                  *pjob,
  struct batch_request *req,
  int                   status)

  {
  char         log_buf[LOCAL_LOG_BUF_SIZE];

  int          newstate;
  int          newsub;

  if (req->rq_type != PBS_BATCH_MoveJob)
    {
    sprintf(log_buf, "bad request type %d\n", req->rq_type);

    log_err(-1, __func__, log_buf);

    return;
    }

  switch (status)
    {
    case LOCUTION_SUCCESS:

      /* purge server's job structure */
      if (pjob->ji_qs.ji_svrflags & JOB_SVFLG_StagedIn)
        remove_stagein(&pjob);

      if (pjob != NULL)
        {
        if (pjob->ji_qs.ji_svrflags & JOB_SVFLG_CHECKPOINT_COPIED)
          remove_checkpoint(&pjob);
        }

      snprintf(log_buf, sizeof(log_buf), "%s", msg_movejob);
      snprintf(log_buf + strlen(log_buf), sizeof(log_buf) - strlen(log_buf), msg_manager,
        req->rq_ind.rq_move.rq_destin, req->rq_user, req->rq_host);

      if (pjob != NULL)
        svr_job_purge(pjob);
    
      reply_ack(req);

      break;
  
    default:

      status = PBSE_ROUTEREJ;

      if (pjob != NULL)
        {
        /* force re-eval of job state out of Transit */
        svr_evaljobstate(pjob, &newstate, &newsub, 1);
        svr_setjobstate(pjob, newstate, newsub, FALSE);
   
        unlock_ji_mutex(pjob, __func__, "3", LOGLEVEL);
        }

      req_reject(status, 0, req, NULL, NULL);
    } /* END switch (status) */
  
  } /* END finish_moving_processing() */





void finish_move_process(

  char                 *job_id,
  struct batch_request *preq,
  long                  time,
  char                 *node_name,
  int                   status,
  int                   type,
  int                   mom_err)

  {
  char  log_buf[LOCAL_LOG_BUF_SIZE+1];
  job  *pjob = svr_find_job(job_id);

  if (pjob == NULL)
    {
    /* somehow the job has been deleted mid-runjob */
    snprintf(log_buf, LOCAL_LOG_BUF_SIZE,
      "Job %s was deleted while servicing move request", job_id);

    if (preq != NULL)
      {
      if (mom_err != PBSE_NONE)
        req_reject(mom_err, 0, preq, node_name, log_buf);
      else
        req_reject(PBSE_JOBNOTFOUND, 0, preq, node_name, log_buf);
      }
    }
  else
    {
    switch (type)
      {
      case MOVE_TYPE_Move:
        finish_moving_processing(pjob, preq, status);
        break;
        
      case MOVE_TYPE_Route:
        finish_routing_processing(pjob, status);
        break;
        
      case MOVE_TYPE_Exec:
        unlock_ji_mutex(pjob, __func__, "1", LOGLEVEL);
        pjob = NULL;
        finish_sendmom(job_id, preq, time, node_name, status, mom_err);
        
        break;
      } /* END switch (type) */
    }
  if (pjob != NULL)
    unlock_ji_mutex(pjob, __func__, "2", LOGLEVEL);


  } /* END finish_move_process() */




void free_server_attrs(

  tlist_head *attrl_ptr)

  {
  tlist_head  attrl = *attrl_ptr;
  svrattrl   *pal;
  svrattrl   *tmp;

  pal = (svrattrl *)GET_NEXT(attrl);

  while (pal != NULL)
    {
    tmp = GET_NEXT(pal->al_link);
    delete_link(&pal->al_link);
    free(pal);

    pal = tmp;
    }
  } /* END free_server_attrs() */




int send_job_work(

  char                  *job_id,
  char                  *node_name, /* I */
  int                    type,      /* I */
  int                   *my_err,    /* O */
  struct batch_request  *preq)      /* M */

  {
  int                   rc = LOCUTION_FAIL;
  int                   ret = PBSE_NONE;
  int                   local_errno = 0;
  tlist_head            attrl;
  enum conn_type        cntype = ToServerDIS;
  int                   con = -1;
  int                   sock = -1;
  int                   encode_type;
  int                   mom_err = PBSE_NONE;
  int                   i;
  int                   NumRetries;
  int                   resc_access_perm;
  char                  script_name[MAXPATHLEN + 1];
  char                 *pc;
  char                  stdout_path[MAXPATHLEN + 1];
  char                  stderr_path[MAXPATHLEN + 1];
  char                  chkpt_path[MAXPATHLEN + 1];
  char                  log_buf[LOCAL_LOG_BUF_SIZE];
  long                  start_time = time(NULL);
  long                  sid;
  unsigned char         attempt_to_queue_job = FALSE;
  unsigned char         change_substate_on_attempt_to_queue = FALSE;
  unsigned char         has_job_script = FALSE;
  unsigned char         job_has_run = FALSE;
  job                  *pjob = NULL;
  char                  job_destin[PBS_MAXROUTEDEST+1];

  struct attropl       *pqjatr;      /* list (single) of attropl for quejob */
  pbs_attribute        *pattr;
  mbool_t               Timeout = FALSE;
  
  unsigned long         job_momaddr = -1;
  unsigned short        job_momport = -1;

  if ((pjob = svr_find_job(job_id)) == NULL)
    return(PBSE_JOBNOTFOUND);

  if (strlen(pjob->ji_qs.ji_destin) != 0)
    strcpy(job_destin, pjob->ji_qs.ji_destin);
  else
    job_destin[0] = '\0';

  job_momaddr = pjob->ji_qs.ji_un.ji_exect.ji_momaddr;
  job_momport = pjob->ji_qs.ji_un.ji_exect.ji_momport;

  if (pjob->ji_qs.ji_svrflags & JOB_SVFLG_SCRIPT)
    has_job_script = TRUE;

  if (pjob->ji_qs.ji_svrflags & JOB_SVFLG_HASRUN)
    job_has_run = TRUE;

  if ((job_destin[0] != '\0') && 
      (type != MOVE_TYPE_Exec))
    {
    if ((pc = strchr(job_destin, '@')) != NULL)
      {
      job_momaddr = get_hostaddr(&local_errno, pc + 1);
      job_momport = pbs_server_port_dis;
      }
    }

  /* encode job attributes to be moved */
  CLEAR_HEAD(attrl);

  /* select attributes/resources to send based on move type */
  if (type == MOVE_TYPE_Exec)
    {
    /* moving job to MOM - ie job start */

    resc_access_perm = ATR_DFLAG_MOM;
    encode_type = ATR_ENCODE_MOM;
    cntype = ToServerDIS;
    }
  else
    {
    /* moving job to alternate server? */
    resc_access_perm =
      ATR_DFLAG_USWR |
      ATR_DFLAG_OPWR |
      ATR_DFLAG_MGWR |
      ATR_DFLAG_SvRD;

    encode_type = ATR_ENCODE_SVR;

    /* clear default resource settings */
    unlock_ji_mutex(pjob, __func__, "1", LOGLEVEL);
    ret = svr_dequejob(job_id, FALSE);
    if (ret)
      return(ret);
    }

  pattr = pjob->ji_wattr;

  for (i = 0;i < JOB_ATR_LAST;i++)
    {
    if (((job_attr_def + i)->at_flags & resc_access_perm) ||
        ((strncmp((job_attr_def + i)->at_name,"session_id",10) == 0) &&
         (pjob->ji_wattr[JOB_ATR_checkpoint_name].at_flags & ATR_VFLAG_SET)))
      {
      (job_attr_def + i)->at_encode(
        pattr + i,
        &attrl,
        (job_attr_def + i)->at_name,
        NULL,
        encode_type,
        resc_access_perm);
      }
    }    /* END for (i) */

  attrl_fixlink(&attrl);

  /* put together the job script file name */
  if (pjob->ji_arraystruct != NULL)
    {
    snprintf(script_name, sizeof(script_name), "%s%s%s",
      path_jobs, pjob->ji_arraystruct->ai_qs.fileprefix, JOB_SCRIPT_SUFFIX);
    }
  else
    {
    snprintf(script_name, sizeof(script_name), "%s%s%s",
      path_jobs, pjob->ji_qs.ji_fileprefix, JOB_SCRIPT_SUFFIX);
    }
  
  if (job_has_run)
    {
    if ((get_job_file_path(pjob, StdOut, stdout_path, sizeof(stdout_path)) != 0) ||
        (get_job_file_path(pjob ,StdErr, stderr_path, sizeof(stderr_path)) != 0) ||
        (get_job_file_path(pjob, Checkpoint, chkpt_path, sizeof(chkpt_path)) != 0))
      {
      unlock_ji_mutex(pjob, __func__, "2", LOGLEVEL);
      goto send_job_work_end;
      }
    }

  /* if the job is substate JOB_SUBSTATE_TRNOUTCM it means we are 
   * recovering after being down or a late failure so we just want 
   * to send the "ready-to-commit/commit" */
  if (pjob->ji_qs.ji_substate != JOB_SUBSTATE_TRNOUTCM)
    {
    attempt_to_queue_job = TRUE;

    if (pjob->ji_qs.ji_substate != JOB_SUBSTATE_TRNOUT)
      change_substate_on_attempt_to_queue = TRUE;
    }
  
  unlock_ji_mutex(pjob, __func__, "3", LOGLEVEL);

  for (NumRetries = 0;NumRetries < RETRY;NumRetries++)
    {
    /* connect to receiving server with retries */
    if (NumRetries > 0)
      {
      /* recycle after an error */
      if (con >= 0)
        {
        svr_disconnect(con);

        close_conn(sock, FALSE);
        }

      /* check my_err from previous attempt */
      if (should_retry_route(*my_err) == -1)
        {
        sprintf(log_buf, "child failed in previous commit request for job %s", job_id);

        log_err(*my_err, __func__, log_buf);
        goto send_job_work_end;
        }

      sleep(1 << NumRetries);
      }

    if ((con = svr_connect(job_momaddr, job_momport, my_err, NULL, NULL, cntype)) == PBS_NET_RC_FATAL)
      {
      sprintf(log_buf, "send_job failed to host %s, %lx port %d",
        (job_destin[0] != '\0') ? job_destin : "unknown host",
        job_momaddr,
        job_momport);

      log_err(*my_err, __func__, log_buf);
      goto send_job_work_end;
      }

    if (con == PBS_NET_RC_RETRY)
      {
      *my_err = 0; /* should retry */

      continue;
      }

    pthread_mutex_lock(connection[con].ch_mutex);
    sock = connection[con].ch_socket;
    pthread_mutex_unlock(connection[con].ch_mutex);

    if (attempt_to_queue_job == TRUE)
      {
      if (change_substate_on_attempt_to_queue == TRUE)
        {
        if ((pjob = svr_find_job(job_id)) != NULL)
          {
          pjob->ji_qs.ji_substate = JOB_SUBSTATE_TRNOUT;
          job_save(pjob, SAVEJOB_QUICK, 0);
          unlock_ji_mutex(pjob, __func__, "4", LOGLEVEL);
          }
        else
          break;
        }

      pqjatr = &((svrattrl *)GET_NEXT(attrl))->al_atopl;

      if ((pc = PBSD_queuejob(con, my_err, job_id, job_destin, pqjatr, NULL)) == NULL)
        {
        if (*my_err == PBSE_EXPIRED)
          {
          /* queue job timeout based on pbs_tcp_timeout */

          Timeout = TRUE;
          }

        if ((*my_err == PBSE_JOBEXIST) && (type == MOVE_TYPE_Exec))
          {
          /* already running, mark it so */
          log_event(PBSEVENT_ERROR, PBS_EVENTCLASS_JOB, job_id,
            "MOM reports job already running");
          rc = PBSE_NONE; /* Equivalent to LOCUTION_SUCCESS */
          break;
          }

        sprintf(log_buf, "send of job to %s failed error = %d",
          (job_destin[0] != '\0') ? job_destin : "unknown host",
          *my_err);

        log_event(PBSEVENT_JOB, PBS_EVENTCLASS_JOB, job_id, log_buf);

        continue;
        }  /* END if ((pc = PBSD_queuejob() == NULL) */

      free(pc);

      if (has_job_script == TRUE)
        {
        if (PBSD_jscript(con, script_name, job_id) != 0)
          continue;
        }

      /* XXX may need to change the logic below, if we are sending the job to
         a mom on the same host and the mom and server are not sharing the same
         spool directory, then we still need to move the file */

      if ((type == MOVE_TYPE_Exec) &&
          (job_has_run == TRUE) &&
          (job_momaddr != pbs_server_addr))
        {
        /* send files created on prior run */
        if ((PBSD_jobfile(con, PBS_BATCH_MvJobFile, stdout_path, job_id, StdOut) != PBSE_NONE) ||
            (PBSD_jobfile(con, PBS_BATCH_MvJobFile, stderr_path, job_id, StdErr) != PBSE_NONE) ||
            (PBSD_jobfile(con, PBS_BATCH_MvJobFile, chkpt_path, job_id, Checkpoint) != PBSE_NONE))
          {
          continue;
          }
        }

      if ((pjob = svr_find_job(job_id)) != NULL)
        {
        pjob->ji_qs.ji_substate = JOB_SUBSTATE_TRNOUTCM;      
        job_save(pjob, SAVEJOB_QUICK, 0);
        unlock_ji_mutex(pjob, __func__, "5", LOGLEVEL);
        }
      else
        break;

      attempt_to_queue_job = FALSE;
      }

    if (PBSD_rdytocmt(con, job_id) != 0)
      {
      continue;
      }

    if ((mom_err = PBSD_commit_get_sid(con, &sid, job_id)) != PBSE_NONE)
      {
      int   errno2;
      char *err_text;

      pthread_mutex_lock(connection[con].ch_mutex);
      err_text = connection[con].ch_errtxt;
      pthread_mutex_unlock(connection[con].ch_mutex);

      /* NOTE:  errno is modified by log_err */
      if (mom_err > PBSE_FLOOR)
        {
        sprintf(log_buf, "send_job commit failed, rc=%d (%s: %s)",
          mom_err, pbse_to_txt(mom_err), (err_text != NULL) ? err_text : "N/A");
        errno2 = mom_err;
        }
      else
        {
        sprintf(log_buf, "send_job commit failed, rc=%d (%s)",
          mom_err, (err_text != NULL) ? err_text : "N/A");
        errno2 = errno;
        }

      log_ext(errno2, __func__, log_buf, LOG_WARNING);

      /* if failure occurs, pbs_mom should purge job and pbs_server should set *
         job state to idle w/error msg */

      if (errno2 == EINPROGRESS)
        {
        /* request is still being processed */

        /* increase tcp_timeout in qmgr? */

        Timeout = TRUE;

        /* do we need a continue here? */

        sprintf(log_buf, "child commit request timed-out for job %s, increase tcp_timeout?",
          job_id);

        log_ext(errno2, __func__, log_buf, LOG_WARNING);

        /* don't retry on timeout--break out and report error! */
        rc = LOCUTION_FAIL;
        break;
        }
      else
        {
        sprintf(log_buf, "child failed in commit request for job %s", job_id);

        log_ext(errno2, __func__, log_buf, LOG_CRIT);

        /* FAILURE */
        rc = LOCUTION_FAIL;
        break;
        }
      } /* END if ((rc = PBSD_commit(con,job_id)) != 0) */
    else if (sid != -1)
      {
      /* save the sid */
      if ((pjob = svr_find_job(job_id)) != NULL)
        {
        pjob->ji_wattr[JOB_ATR_session_id].at_val.at_long = sid;
        pjob->ji_wattr[JOB_ATR_session_id].at_flags |= ATR_VFLAG_SET;
        unlock_ji_mutex(pjob, __func__, "6", LOGLEVEL);
        }
      else
        {
        rc = LOCUTION_FAIL;
        break;
        }
      }

    svr_disconnect(con);

    close_conn(sock, FALSE);

    /* SUCCESS */
    rc = PBSE_NONE;  /* Equivalent value to LOCUTION_SUCCESS */
    goto send_job_work_end;
    }  /* END for (NumRetries) */
        
  if (con >= 0)
    {
    svr_disconnect(con);

    close_conn(sock, FALSE);
    }

  if (Timeout == TRUE)
    {
    /* 10 indicates that job migrate timed out, server will mark node down *
          and abort the job - see post_sendmom() */
    sprintf(log_buf, "child timed-out attempting to start job %s", job_id);
    log_ext(*my_err, __func__, log_buf, LOG_WARNING);
    rc = LOCUTION_REQUEUE;
    }
  else if (should_retry_route(*my_err) == -1)
    {
    sprintf(log_buf, "child failed and will not retry job %s", job_id);
    log_err(*my_err, __func__, log_buf);
    rc = LOCUTION_FAIL;
    }
  else
    rc = LOCUTION_REQUEUE;

send_job_work_end:

  finish_move_process(job_id, preq, start_time, node_name, rc, type, mom_err);
  free_server_attrs(&attrl);

  return(rc);
  } /* END send_job_work() */




/*
 * send_job - send a job over the network to some other server or MOM
 *
 * Start a child to do the work.  Connect to the destination host and port,
 * and go through the protocol to transfer the job.
 *
 * @see svr_strtjob2() - parent
 *
 * Child exit status:
 *  0 success, job sent
 *  1 permanent failure or rejection
 *  2 failed but try again
 */

void *send_job(

  void *vp) /* I: jobid, hostaddr, port, move_type, and data */

  {
  send_job_request     *args = (send_job_request *)vp;
  char                 *job_id = args->jobid;
  job                  *pjob;
  int                   type = args->move_type; /* move, route, or execute */
  int                   local_errno = 0;
  char                  log_buf[LOCAL_LOG_BUF_SIZE];
  unsigned long         job_momaddr;

  char                 *node_name = NULL;

  struct batch_request *preq = (struct batch_request *)args->data;
  struct pbsnode       *np;

  pjob = svr_find_job(job_id);

  if (pjob != NULL)
    {
    job_momaddr = pjob->ji_qs.ji_un.ji_exect.ji_momaddr;
    unlock_ji_mutex(pjob, __func__, "1", LOGLEVEL);

    if (LOGLEVEL >= 6)
      {
      sprintf(log_buf,"about to send job - type=%d",type);
      
      log_event(PBSEVENT_JOB,PBS_EVENTCLASS_JOB,job_id,log_buf);
      }
    
    if ((np = PGetNodeFromAddr(job_momaddr)) != NULL)
      {
      node_name = np->nd_name;
      
      unlock_node(np, "send_job", NULL, LOGLEVEL);
      }
    
    send_job_work(job_id,node_name,type,&local_errno,preq);
    }

  return(NULL);
  }  /* END send_job() */





/*
 * net_move - move a job over the network to another queue.
 *
 * Get the address of the destination server and call send_job()
 *
 * Returns: 2 on success (child started, see send_job()), -1 on error
 */

int net_move(

  job                  *jobp,
  struct batch_request *req)

  {
  void             *data;
  char             *destination = jobp->ji_qs.ji_destin;
  int               move_type;
  char             *toserver;
  send_job_request *args;
  char              log_buf[LOCAL_LOG_BUF_SIZE];

  /* Determine to whom are we sending the job */

  if ((toserver = strchr(destination, '@')) == NULL)
    {
    sprintf(log_buf, "no server specified in %s\n", destination);

    log_err(-1, __func__, log_buf);

    return(-1);
    }

  toserver++;  /* point to server name */

  if (req)
    {
    /* note, in this case, req is the orginal Move Request */
    move_type = MOVE_TYPE_Move;

    data      = req;
    }
  else
    {
    /* note, in this case req is NULL */
    move_type = MOVE_TYPE_Route;

    data      = NULL;
    }

  svr_setjobstate(jobp, JOB_STATE_TRANSIT, JOB_SUBSTATE_TRNOUT, TRUE);

  args = calloc(1, sizeof(send_job_request));

  if (args != NULL)
    {
    args->jobid = jobp->ji_qs.ji_jobid;
    args->move_type = move_type;
    args->data = data;
    }
  else
    {
    /* FAILURE */
    log_err(ENOMEM,"req_runjob","Cannot allocate space for arguments");
    
    return(ENOMEM);
    }

  return(enqueue_threadpool_request(send_job,args));
  }  /* END net_move() */





/*
 * should_retry_route - should the route be retried based on the error return
 *
 * Certain error are temporary, and that destination should not be
 * considered bad.
 *
 * Return:  1 if should retry this destination
 *  -1 if destination should not be retried
 */

static int should_retry_route(

  int err)

  {
  switch (err)
    {
    case 0:

    case EADDRINUSE:

    case EADDRNOTAVAIL:

    case PBSE_SYSTEM:

    case PBSE_INTERNAL:

    case PBSE_EXPIRED:

    case PBSE_MAXQUED:

    case PBSE_MAXUSERQUED:

    case PBSE_QUNOENB:

    case PBSE_NOCONNECTS:

      /* retry destination */

      return(1);

      /*NOTREACHED*/

      break;

    default:

      /* NO-OP */

      break;
    }

  return(-1);
  }  /* END should_retry_route() */





int get_job_file_path(

  job           *pjob,
  enum job_file  which,
  char          *path,
  int            path_size)

  {
  snprintf(path, path_size, "%s%s", path_spool, pjob->ji_qs.ji_fileprefix);

  if (path_size - strlen(path) < 4)
    return(-1);

  if (which == StdOut)
    strcat(path, JOB_STDOUT_SUFFIX);
  else if (which == StdErr)
    strcat(path, JOB_STDERR_SUFFIX);
  else if (which == Checkpoint)
    strcat(path, JOB_CHECKPOINT_SUFFIX);

  if (access(path, F_OK) < 0)
    {
    if (errno != ENOENT)
      return(errno);
    else
      path[0] ='\0';
    }

  return(PBSE_NONE);
  } /* END get_job_file_path() */



