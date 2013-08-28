#ifndef LIBJSON_MOM_STATUS_HPP
#define LIBJSON_MOM_STATUS_HPP

#include <pbs_config.h>

#ifdef ZMQ

#include "Message.hpp"

namespace TrqJson {

  /**
   * Message class implementation specific for MOM Status messages.
   */
  class MomStatusMessage : public Message
    {
    /* Friend class for unit testing */
    friend class TestHelper;

    private:

      /**
       * MOM statuses container. This keeps all received statuses and this MOM status.
       * It have to be cleared after the message is sent.
       */
      std::map<std::string, Json::Value> statusMap;

      /**
       * Helper function that handles GPU status part of the status message.
       * @param statusStrings dynamic status strings. The first one have to be <gpu_status>
       * @param status value where GPU status should be added.
       * @return the pointer to the last string in the GPU status that is </gpu_status>
       */
      const char *readStringGpuStatus(const char *statusStrings, Json::Value &status);

      /**
       * Helper function that handles MIC status part of the status message.
       * @param statusStrings dynamic status strings. The first one have to be <mic_status>
       * @param status value where MIC status should be added.
       * @return the pointer to the last string in the MIC status that is </mic_status>
       */
      const char *readStringMicStatus(const char *statusStrings, Json::Value &status);

    protected:

      /**
       * Put all collected statuses to the given Json Value.
       * @param messageBody the value to be updated
       */
      void generateBody(Json::Value &messageBody);

      /**
       * Return the message type. It's always "status" string.
       * @return this message type.
       */
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
