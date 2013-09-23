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
    int pbsReadJsonStatus(const size_t sz, const char *data);

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
     * Process GPU part of the status
     * @param gpusStatus a value object containing the node GPUs status
     * @return 0 if succeeded, -1 otherwise
     */
    int pbsReadJsonGpuStatus(Json::Value &gpusStatus);
    /**
     * Process MIC part of the status
     * @param micsStatus a value object containing the node MICs status
     * @return 0 if succeeded, -1 otherwise
     */
    int pbsReadJsonMicStatus(Json::Value &micsStatus);
  };

}

#endif /* ZMQ */

#endif /* _ZMQ_PROCESS_MOM_COMMUNICATION_H */
