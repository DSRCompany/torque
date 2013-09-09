#include "log.h"
#include "Message.hpp"

char log_buffer[LOG_BUF_SIZE];

void log_err(int errnum, const char *routine, const char *text)
  {
  // Do nothing, assume never fails
  }

void log_record(int eventtype, int objclass, const char *objname, const char *text)
  {
  // Do nothing, assume never fails
  }
