#ifndef _ZMQ_PROCESS_MOM_COMMUNICATION_H
#define _ZMQ_PROCESS_MOM_COMMUNICATION_H

#include <pbs_config.h>

#ifdef ZMQ

#include <jsoncpp/json/json.h>

#include "pbs_nodes.h"

namespace TrqZStatus {

/**
 * A class processing MOM status update requests.
 */
class MomUpdate
  {
  /* Friend class for unit testing */
  friend class TestHelper;

  public:
    /**
     * Process json status from the given buffer.
     * @param sz the message buffer size in bytes
     * @param data pointer to the json data buffer
     * @return 0 if succeeded, -1 otherwise
     */
    int readJsonStatus(const size_t sz, const char *data);

  private:
    /**
     * Json message reader
     */
    Json::Reader m_reader;
    /**
     * A pointer to the currently being processed node
     */
    struct pbsnode *m_current_node;
    /**
     * Local cache of server properties
     */
    long m_mom_job_sync;
    long m_auto_np;
    long m_down_on_error;

    /**
     * Process GPU part of the status
     * @param gpusStatus a value object containing the node GPUs status
     * @return 0 if succeeded, -1 otherwise
     */
    int processGpuStatus(Json::Value &gpusStatus);
    /**
     * Process MIC part of the status
     * @param micsStatus a value object containing the node MICs status
     * @return 0 if succeeded, -1 otherwise
     */
    int processMicStatus(Json::Value &micsStatus);
    /**
     * Process one node status values
     * @param nodeStatus a value object containing one node status
     * @return 0 if succeeded, -1 otherwise
     */
    int processNodeStatus(Json::Value &nodeStatus);
  };

}

#endif /* ZMQ */

#endif /* _ZMQ_PROCESS_MOM_COMMUNICATION_H */
