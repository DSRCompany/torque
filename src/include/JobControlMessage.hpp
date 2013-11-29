#ifndef LIBJSON_JOB_CONTROL_HPP
#define LIBJSON_JOB_CONTROL_HPP

#include <pbs_config.h>

#ifdef ZMQ

#include <boost/ptr_container/ptr_vector.hpp>
#include "Message.hpp"

namespace TrqJson {

  /**
   * Message class implementation specific for MOM Status messages.
   */
  class JobControlMessage : public Message
    {
    /* Friend class for unit testing */
    friend class TestHelper;

    public:

      /* Getters and setters */
      std::string JobControlMessage::getMessageType();
      void JobControlMessage::setMessageType(std::string messageType);
      std::string JobControlMessage::getCommand();
      void JobControlMessage::setCommand(std::string command);
      std::string JobControlMessage::getJobId();
      void JobControlMessage::setJobId(std::string jobId);
      std::string JobControlMessage::getCookie();
      void JobControlMessage::setCookie(std::string cookie);
      std::string JobControlMessage::getEvent();
      void JobControlMessage::setEvent(std::string event);
      std::string JobControlMessage::getTaskId();
      void JobControlMessage::setTaskId(std::string taskId);
      std::string JobControlMessage::getError();
      void JobControlMessage::setError(std::string error);

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

    private:

      /* private fields */
      std::string  messageType;
      std::string  command;
      std::string  jobId;
      std::string  cookie;
      int          event;
      unsigned int taskId;
      int          error;

    }; /* class JobControlMessage */

} /* namespace TrqJson */

#endif /* ZMQ */

#endif /* LIBJSON_JOB_CONTROL_HPP */
