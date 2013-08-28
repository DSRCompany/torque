#include <pbs_config.h>

#ifdef ZMQ

#include <map>
#include <string>
#include <jsoncpp/json/json.h>

#include "MomStatusMessage.hpp"
#include "mom_comm.h"



/**
 * Global map container to keep this MOM and all received MOM statuses.
 * The statuses are kept as is as Json::Value object.
 * The key in the map is node identifier aka name.
 */
static TrqJson::MomStatusMessage gs_received_json_statuses;

extern char mom_alias[];
extern int  updates_waiting_to_send;
extern int  maxupdatesbeforesending;



/**
 * Update this MOM status message in the global statuses map with the values from the given
 * status_strings dynamic string.
 * @param status_strings dynamic string containing this MOM status.
 */
void update_my_json_status(char *status_strings)
  {
  gs_received_json_statuses.readMergeStringStatus(mom_alias, status_strings);
  } /* END update_my_json_status() */



int init_msg_json_status(zmq_msg_t *message)
  {
  // TODO: handle possible exceptions (std::bad_alloc)
  const char *message_data;
  std::string *message_string;

  gs_received_json_statuses.setMomId(mom_alias);
  message_string = gs_received_json_statuses.write();
  message_data = message_string->c_str();

  return zmq_msg_init_data(message, (void *)message_data, message_string->length(),
      gs_received_json_statuses.deleteString, message_string);
  }

#endif /* ZMQ */
