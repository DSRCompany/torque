#include <pbs_config.h>

#ifdef ZMQ

#include <string>
#include <cstring>

#include "MomStatusMessage.hpp"
#include "log.h"


namespace TrqJson {



int MomStatusMessage::readMergeJsonStatuses(const size_t size, const char *data)
  {
  if (size == 0 || !data) 
    {
    return -1;
    }

  // read
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(data, data + size - 1, root, false);
  if ( !parsingSuccessful )
    {
    // Can't read the message. Malformed?
    return -1;
    }

  if (root["messageType"].asString() != "status")
    {
    // There is no 'messageType' key in the message
    return -1;
    }

  const Json::Value body = root["body"];

  if (!body.isArray())
    {
    // Body have to be an array of nodes statuses
    return -1;
    }

  int updatesCount = 0;
  for (auto nodeStatus : body)
    {
    // TODO: make const string
    std::string nodeId = nodeStatus["node"].asString();
    if (!nodeId.empty())
      {
      updatesCount++;
      statusMap[nodeId] = nodeStatus;
      }
    }

  return(updatesCount);

  } /* END mom_read_json_status() */



void MomStatusMessage::readMergeStringStatus(const char *nodeId, const char *statusStrings)
  {

  Json::Value myStatus(Json::objectValue);

  myStatus[JSON_NODE_ID_KEY] = nodeId;
  for (const char *keyPtr = statusStrings; keyPtr && *keyPtr; keyPtr += strlen(keyPtr) + 1)
    {
    // Split each key-value pair by '=' character and set Json values.
    const char *valPtr = strchr(keyPtr, '=');
    if (!valPtr)
      {
      sprintf(log_buffer,"skipping non key-value pair \"%s\"", keyPtr);
      log_err(0, __func__, log_buffer);
      continue;
      }

    std::string key(keyPtr, valPtr - keyPtr);
    valPtr++; // Move beyond the '=' character
    myStatus[key] = valPtr;
    }

    statusMap[nodeId] = myStatus;
  } /* MomStatusMessage::readMergeStringStatus() */



void MomStatusMessage::generateBody(Json::Value &messageBody)
  {
  for (auto status : statusMap)
    {
    messageBody.append(status.second);
    }
  } /* MomStatusMessage::createBody() */



std::string MomStatusMessage::getMessageType()
  {
  return "status";
  } /* MomStatusMessage::getMessageType() */



} /* namespace TrqJson */

#endif /* ZMQ */
