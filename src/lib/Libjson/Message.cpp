#include <pbs_config.h>

#ifdef ZMQ

#include <string>
#include <jsoncpp/json/json.h>
#include <string.h>

#include "Message.hpp"

namespace TrqJson {

std::string Message::generateUuid()
  {
  char buf[] = "f81d4fae-7dec-11d0-a765-00a0c91e6bf6"; // an example UUID
  const int div = RAND_MAX / 0xffff;

  sprintf(buf, "%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
      rand() / div, rand() / div, rand() / div, rand() / div,
      rand() / div, rand() / div, rand() / div, rand() / div);
  return buf;
  }



std::string Message::getIso8601Time()
  {
  time_t rawTime;
  struct tm * timeInfo;
  char buffer [] = "2013-07-17 15:09:34.123+0400"; // an example date
  const size_t bufferSize = sizeof(buffer);

  time(&rawTime);
  timeInfo = localtime(&rawTime);

  strftime(buffer, bufferSize, "%F %T.000%z", timeInfo);

  return buffer;
  }



void Message::generateHeader(Json::Value &root)
  {
  root["messageId"] = generateUuid();
  root["messageType"] = getMessageType();
  root["ttl"] = 3000;
  root["sentDate"] = getIso8601Time();
  root["senderId"] = momId;
  }



void Message::write(std::string &out)
  {
  Json::Value root;
  generateHeader(root);
  generateBody(root["body"]);

#ifdef DEBUG
  Json::StyledWriter writer;
#else /* DEBUG */
  Json::FastWriter writer;
#endif /* DEBUG */
  
  // Write the message and return char buffer
  out = writer.write(root);
  }



void Message::deleteString(void *data, void *hint)
  {
  (void)data; /* avoid 'unused' compiler warning */
  delete((std::string *)hint);
  }



void Message::setMomId(std::string momId)
  {
  this->momId = momId;
  }

} /* namespace TrqJson */

#endif /* ZMQ */
