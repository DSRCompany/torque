#include "log.h"
#include "Message.hpp"

char log_buffer[LOG_BUF_SIZE];

void log_err(int errnum, const char *routine, const char *text)
  {
  (void)errnum; (void)routine; (void)text;
  }
