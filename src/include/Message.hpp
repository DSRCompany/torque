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

  class Message
    {

  private:
    std::string momId;
    
    /**
     * Generates UUID.
     * NOTE: current implementation just generates random sequence. For better security libuuid should
     * be used.
     * @return random string in UUID format.
     */
    static std::string generateUuid();

    /**
     * Gets the current timestamp represented in ISO 8601 format in local time with timezone offset.
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
    virtual void generateBody(Json::Value &messageBody) = 0;
    virtual std::string getMessageType() = 0;

  public:
    void setMomId(std::string momId);

    /**
     * Dump all collected Json status messages into a character buffer.
     * @return heap char array that have to be deallocated with free().
     */
    void write(std::string &out);

    /**
     * Deallocate character buffer.
     * The function is designed to be passed to zmq_msg_init_data().
     * @param data the buffer to be deallocated.
     * @param hint isn't used.
     */
    static void deleteString(void *data, void *hint);
    }; /* class Message */

} /* namespace TrqJson */

#endif /* ZMQ */

#endif /* LIBJSON_MOM_STATUS_HPP */
