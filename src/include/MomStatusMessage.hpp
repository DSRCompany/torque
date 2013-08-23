#ifndef LIBJSON_MOM_STATUS_HPP
#define LIBJSON_MOM_STATUS_HPP

#include <pbs_config.h>

#ifdef ZMQ

#include "Message.hpp"

namespace TrqJson {

  class MomStatusMessage : public Message
    {
  private:
    std::map<std::string, Json::Value> statusMap;

  protected:
    void generateBody(Json::Value &messageBody);
    std::string getMessageType();

  public:
    /**
     * Parse and handle mom status message from the given buffer.
     * @param sz data buffer size.
     * @param data message data buffer.
     * @return 0 if succeeded or -1 otherwise.
     */
    int readMergeJsonStatuses(const size_t size, const char *data);

    /**
     * Update this MOM status message in the global statuses map with the values from the given
     * status_strings dynamic string.
     * @param status_strings dynamic string containing this MOM status.
     */
    void readMergeStringStatus(const char *nodeId, const char *statusStrings);

    }; /* class MomStatusMessage */

} /* namespace TrqJson */

#endif /* ZMQ */

#endif /* LIBJSON_MOM_STATUS_HPP */
