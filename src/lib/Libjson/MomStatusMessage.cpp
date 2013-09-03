#include <pbs_config.h>

#ifdef ZMQ

#include <string>
#include <cstring>

#include "MomStatusMessage.hpp"
#include "log.h"
#include "pbs_nodes.h"


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
  bool parsingSuccessful = reader.parse(data, data + size, root, false);
  if ( !parsingSuccessful )
    {
    // Can't read the message. Malformed?
    return -1;
    }

  Json::Value &messageType = root["messageType"];
  if (!messageType.isString())
    {
    return -1;
    }
  if (messageType.asString() != "status")
    {
    // There is no 'messageType' key in the message
    return -1;
    }

  const Json::Value &body = root["body"];

  if (!body.isArray())
    {
    // Body have to be an array of nodes statuses
    return -1;
    }

  int updatesCount = 0;
  for (unsigned int i = 0; i < body.size(); i++)
    {
    std::string nodeId = body[i][JSON_NODE_ID_KEY].asString();
    if (!nodeId.empty())
      {
      updatesCount++;
      statusMap[nodeId] = body[i];
      }
    }

  return(updatesCount);

  } /* END mom_read_json_status() */



const char *MomStatusMessage::readStringGpuStatus(const char *statusStrings, Json::Value &status)
  {
  if (strcmp(statusStrings, START_GPU_STATUS))
    {
    return statusStrings; // Non-gpu status
    }

  Json::Value &gpuStatuses = status[GPU_STATUS_KEY];
  Json::Value currentGpu;
  std::string timestamp;
  std::string driverVer;

  const char *keyPtr = statusStrings + strlen(statusStrings) + 1;
  for (; keyPtr && *keyPtr; keyPtr += strlen(keyPtr) + 1)
    {
    // Split each key-value pair by '=' character and set Json values.
    const char *valPtr = strchr(keyPtr, '=');
    if (!valPtr)
      {
      if (!strcmp(keyPtr, END_GPU_STATUS))
        {
        break;
        }
      // else
      sprintf(log_buffer,"skipping unknown non key-value \"%s\"", keyPtr);
      log_err(0, __func__, log_buffer);
      continue;
      }

    std::string key(keyPtr, valPtr - keyPtr);
    valPtr++; // Move beyond the '=' character

    // get timestamp or driver version
    if (!key.compare("timestamp"))
      {
      timestamp = valPtr;
      continue;
      }
    if (!key.compare("driver_ver"))
      {
      driverVer = valPtr;
      continue;
      }

    // start of new gpu status
    if (!key.compare("gpuid") && !currentGpu.empty())
      {
      currentGpu["timestamp"] = timestamp;
      currentGpu["driver_ver"] = driverVer;
      gpuStatuses.append(currentGpu);
      currentGpu.clear();
      }
    currentGpu[key] = valPtr;
    }

  if (!currentGpu.empty())
    {
    gpuStatuses.append(currentGpu);
    }

  return keyPtr;
  }



const char *MomStatusMessage::readStringMicStatus(const char *statusStrings, Json::Value &status)
  {
  if (strcmp(statusStrings, START_MIC_STATUS))
    {
    return statusStrings; // Non-gpu status
    }

  Json::Value &micStatus = status[MIC_STATUS_KEY];
  Json::Value currentMic;

  const char *keyPtr = statusStrings + strlen(statusStrings) + 1;
  for (; keyPtr && *keyPtr; keyPtr += strlen(keyPtr) + 1)
    {
    // Split each key-value pair by '=' character and set Json values.
    const char *valPtr = strchr(keyPtr, '=');
    if (!valPtr)
      {
      if (!strcmp(keyPtr, END_MIC_STATUS))
        {
        break;
        }
      // else
      sprintf(log_buffer,"skipping unknown non key-value \"%s\"", keyPtr);
      log_err(0, __func__, log_buffer);
      continue;
      }

    std::string key(keyPtr, valPtr - keyPtr);
    valPtr++; // Move beyond the '=' character

    // start of new mic status
    if (!key.compare("mic_id") && !currentMic.empty())
      {
      micStatus.append(currentMic);
      currentMic.clear();
      }
    currentMic[key] = valPtr;
    }

  if (!currentMic.empty())
    {
    micStatus.append(currentMic);
    }

  return keyPtr;
  }



void MomStatusMessage::readMergeStringStatus(const char *nodeId, const char *statusStrings, bool request_hierarchy)
  {
  if (nodeId == NULL || strlen(nodeId) == 0 || statusStrings == NULL || strlen(statusStrings) == 0)
    {
    return;
    }

  Json::Value myStatus(Json::objectValue);

  myStatus[JSON_NODE_ID_KEY] = nodeId;
  if (request_hierarchy)
    {
      myStatus["first_update"] = true;
    }
  for (const char *keyPtr = statusStrings; keyPtr && *keyPtr; keyPtr += strlen(keyPtr) + 1)
    {
    // Split each key-value pair by '=' character and set Json values.
    const char *valPtr = strchr(keyPtr, '=');
    if (!valPtr)
      {
      const char *newPtr;
      newPtr = readStringGpuStatus(keyPtr, myStatus);
      if (newPtr == keyPtr)
        {
        newPtr = readStringMicStatus(keyPtr, myStatus);
        }
      if (newPtr == keyPtr) // Non GPU and non MIC
        {
        sprintf(log_buffer,"skipping unknown non key-value \"%s\"", keyPtr);
        log_err(0, __func__, log_buffer);
        }
      else
        {
        keyPtr = newPtr;
        }
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
  for (std::map<std::string, Json::Value>::const_iterator it = statusMap.begin(); it != statusMap.end(); it++)
    {
    messageBody.append((*it).second);
    }
  } /* MomStatusMessage::createBody() */



std::string MomStatusMessage::getMessageType()
  {
  return "status";
  } /* MomStatusMessage::getMessageType() */



void MomStatusMessage::clear()
  {
  statusMap.clear();
  }

} /* namespace TrqJson */

#endif /* ZMQ */
