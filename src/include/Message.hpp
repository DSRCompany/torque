#ifndef LIBJSON_STATUS_UTIL_HPP
#define LIBJSON_STATUS_UTIL_HPP

#include <pbs_config.h>

#ifdef ZMQ

#include <string>
#include <jsoncpp/json/json.h>

/**
 * Json key for node ID
 */
#define JSON_NODE_ID_KEY "node"

namespace TrqJson {

  /**
   * Base class for all Torque Json messages.
   * The class is designed to use with ZeroMQ messages and provides two main public methods for
   * this: write() that serializes this message data to the given string and deleteString() routine
   * that deallocates the string and have to be passed to ZMQ message on initialization state.
   */
  class Message
    {

    friend class TestHelper;

    private:

      /**
       * The ID of the MOM owning this message.
       */
      std::string momId;

      /**
       * Generates UUID.
       * NOTE: current implementation just generates random sequence. For better security libuuid
       * should be used.
       * @return random string in UUID format.
       */
      static std::string generateUuid();

      /**
       * Gets the current timestamp represented in ISO 8601 format in local time with timezone
       * offset.
       * @return a string containing the timestamp.
       */
      static std::string getIso8601Time();

      /**
       * Create standard for all Json messages header.
       * @param message_type the type of the message.
       * @return JsonCPP object that could be updated with other specific information.
       */
      void generateHeader(Json::Value &root);

    protected:

      /**
       * The methond have to be implemented by specific implementations. The method should fill the
       * given Json::Value object with corresponding message data.
       * @param messageBody message body value.
       */
      virtual void generateBody(Json::Value &messageBody) = 0;

      /**
       * The method have to be implemented by specific implementations. The method should return 
       * message type string like "status", "event" and so on.
       */
      virtual std::string getMessageType() = 0;

    public:

      /**
       * Set the ID of MOM owning the message. It's needed for "senderId" value.
       */
      void setMomId(std::string momId);

      /**
       * Dump all collected Json status messages into a character buffer.
       * @return new string object that have to be deallocated with deleteString() function passed
       *         as the 'hint' argument.
       */
      std::string *write();

      /**
       * Deallocate character buffer.
       * The function is designed to be passed to zmq_msg_init_data().
       * @param data isn't used.
       * @param hint accept heap std::string argument. Will fail with any other types.
       */
      static void deleteString(void *data, void *hint);

    }; /* class Message */

} /* namespace TrqJson */

#endif /* ZMQ */

#endif /* LIBJSON_MOM_STATUS_HPP */
