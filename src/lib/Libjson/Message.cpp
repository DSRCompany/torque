#include <pbs_config.h>

#ifdef ZMQ

#include <string>
#include <jsoncpp/json/json.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
  root[JSON_SENDER_ID_KEY] = momId;
  }



std::string *Message::write()
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
  std::string *out = new std::string();
  *out = writer.write(root);
  return out;
  }



void Message::deleteString(void *string_data, void *string)
  {
  (void)string_data; /* avoid 'unused' compiler warning */
  if (string == NULL)
    {
    return;
    }
  delete((std::string *)string);
  }



void Message::setMomId(std::string momId)
  {
  this->momId = momId;
  }

} /* namespace TrqJson */

#endif /* ZMQ */
