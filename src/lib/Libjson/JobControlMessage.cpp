#include <pbs_config.h>

#ifdef ZMQ

#include <string>
#include <cstring>

#include "JobControlMessage.hpp"
#include "log.h"
#include "pbs_nodes.h"


namespace TrqJson {



void JobControlMessage::generateBody(Json::Value &messageBody)
  {
  // TODO: implement
  } /* JobControlMessage::createBody() */



std::string JobControlMessage::getMessageType()
  {
  return messageType;
  } /* JobControlMessage::getMessageType() */



void JobControlMessage::setMessageType(std::string messageType)
  {
  this->messageType = messageType;
  } /* JobControlMessage::setMessageType() */



std::string JobControlMessage::getCommand()
  {
  return command;
  } /* JobControlMessage::getCommand() */



void JobControlMessage::setCommand(std::string command)
  {
  this->command = command;
  } /* JobControlMessage::setCommand() */



std::string JobControlMessage::getJobId()
  {
  return jobId;
  } /* JobControlMessage::getJobId() */



void JobControlMessage::setJobId(std::string jobId)
  {
  this->jobId = jobId;
  } /* JobControlMessage::setJobId() */



std::string JobControlMessage::getCookie()
  {
  return cookie;
  } /* JobControlMessage::getCookie() */



void JobControlMessage::setCookie(std::string cookie)
  {
  this->cookie = cookie;
  } /* JobControlMessage::setCookie() */



int JobControlMessage::getEvent()
  {
  return event;
  } /* JobControlMessage::getEvent() */



void JobControlMessage::setEvent(int event)
  {
  this->event = event;
  } /* JobControlMessage::setEvent() */



unsigned int JobControlMessage::getTaskId()
  {
  return taskId;
  } /* JobControlMessage::getTaskId() */



void JobControlMessage::setTaskId(unsigned int taskId)
  {
  this->taskId = taskId;
  } /* JobControlMessage::setTaskId() */



int JobControlMessage::getError()
  {
  return error;
  } /* JobControlMessage::getError() */



void JobControlMessage::setError(int error)
  {
  this->error = error;
  } /* JobControlMessage::setError() */



} /* namespace TrqJson */

#endif /* ZMQ */
