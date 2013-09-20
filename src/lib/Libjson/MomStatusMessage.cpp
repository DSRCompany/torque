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

  Json::Value &senderID = root[JSON_SENDER_ID_KEY];
  if (!senderID.isString())
    {
    return -1;
    }
  sprintf(log_buffer, "Got json statuses message from senderId:%s", senderID.asString().c_str());
  log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);

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
    sprintf(log_buffer, "Got status nodeId:%s", nodeId.c_str());
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, log_buffer);
    }

  return(updatesCount);

  } /* END mom_read_json_status() */



boost::ptr_vector<std::string>::iterator MomStatusMessage::readStringGpuStatus(

  boost::ptr_vector<std::string>::iterator i,
  boost::ptr_vector<std::string> mom_status,
  Json::Value &status)

  {
  if (!i->compare(START_GPU_STATUS))
    {
    i++;
    }
  else
    {
    return i;
    }

  Json::Value &gpuStatuses = status[GPU_STATUS_KEY];
  Json::Value currentGpu;
  bool gpu_status = false;

  while (i != mom_status.end()
         && i->compare(END_GPU_STATUS))
    {
    // Split each key-value pair by '=' character and set Json values.
    std::size_t pos = i->find_first_of("=");    
    if (pos == std::string::npos)
      {
      sprintf(log_buffer,"skipping unknown non key-value \"%s\"", i->c_str());
      log_err(0, __func__, log_buffer);
      i++;
      continue;
      }

    std::string key = i->substr(0, pos);
    std::string value = i->substr(pos+1, std::string::npos);

    if (!key.compare("gpuid"))
      {
      gpu_status = true;
      if (!currentGpu.empty())
        {
        gpuStatuses["gpus"].append(currentGpu);
        currentGpu.clear();
        }
      currentGpu[key.c_str()] = value.c_str();
      }
    else
      {
      if (gpu_status)
        {
        currentGpu[key.c_str()] = value.c_str();
        }
      else
        {
        gpuStatuses[key.c_str()] = value.c_str();
        }
      }
    i++;
    }

  if (!currentGpu.empty())
    {
    gpuStatuses["gpus"].append(currentGpu);
    }

  return i;
  }



boost::ptr_vector<std::string>::iterator MomStatusMessage::readStringMicStatus(

  boost::ptr_vector<std::string>::iterator i,
  boost::ptr_vector<std::string> mom_status,
  Json::Value &status)

  {
  if (!i->compare(START_MIC_STATUS))
    {
    i++;
    }
  else
    {
    return i;
    }

  Json::Value &micStatus = status[MIC_STATUS_KEY];
  Json::Value currentMic;
  bool mic_status = false;

  while (i != mom_status.end()
         && i->compare(END_MIC_STATUS))
    {
    // Split each key-value pair by '=' character and set Json values.
    std::size_t pos = i->find_first_of("=");    
    if (pos == std::string::npos)
      {
      sprintf(log_buffer,"skipping unknown non key-value \"%s\"", i->c_str());
      log_err(0, __func__, log_buffer);
      i++;
      continue;
      }

    std::string key = i->substr(0, pos);
    std::string value = i->substr(pos+1, std::string::npos);


    if (!key.compare("mic_id"))
      {
      mic_status = true;
      if (!currentMic.empty())
        {
        micStatus["mics"].append(currentMic);
        currentMic.clear();
        }
      currentMic[key.c_str()] = value.c_str();
      }
    else
      {
      if (mic_status)
        {
        currentMic[key.c_str()] = value.c_str();
        }
      else
        {
        micStatus[key.c_str()] = value.c_str();
        }
      }
    i++;
    }

  if (!currentMic.empty())
    {
    micStatus["mics"].append(currentMic);
    }

  return i;
  }



void MomStatusMessage::readMergeStringStatus(const char *nodeId, boost::ptr_vector<std::string> mom_status, bool request_hierarchy)
  {
  if (nodeId == NULL || strlen(nodeId) == 0)
    {
    return;
    }

  Json::Value myStatus(Json::objectValue);
  Json::Value currentNuma;

  myStatus[JSON_NODE_ID_KEY] = nodeId;
  if (request_hierarchy)
    {
      myStatus["first_update"] = true;
    }


  boost::ptr_vector<std::string>::iterator i = mom_status.begin();
  while (i != mom_status.end())
    {
    std::size_t pos = i->find_first_of("=");
    if (pos != std::string::npos)
      {
      std::string key = i->substr(0, pos);
      std::string value = i->substr(pos+1, std::string::npos);
      (!currentNuma.empty()) ? (currentNuma[key.c_str()] = value.c_str()) :  myStatus[key.c_str()] = value.c_str();
      }
    else if (!i->compare(START_GPU_STATUS))
      {
      i = readStringGpuStatus(i, mom_status, (!currentNuma.empty()) ? currentNuma : myStatus);
      }
    else if (!i->compare(START_MIC_STATUS))
      {
      i = readStringMicStatus(i, mom_status, (!currentNuma.empty()) ? currentNuma : myStatus);
      }
    else if (!i->compare(NUMA_KEYWORD))
      {
        if (!currentNuma.empty())
        {
        myStatus["numa"].append(currentNuma);
        currentNuma.clear();
        }
        currentNuma["numa"] = i->c_str();
      }
    else
      {
      sprintf(log_buffer,"skipping unknown non key-value \"%s\"", i->c_str());
      log_err(0, __func__, log_buffer);
      }

    if (i != mom_status.end())
      i++;
    }
  
  if (!currentNuma.empty())
    {
    myStatus["numa"].append(currentNuma);
    currentNuma.clear();
    }
  statusMap[nodeId] = myStatus;

  // {
  //   sprintf(log_buffer,"JSON - \"%s\"\n", myStatus.toStyledString().c_str());
  //   log_err(0, __func__, log_buffer);
  // }

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
