#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "Prototype.h"

void writeToLog(const HChar* buf, Int len);
void initLogFile(const HChar* log_filename);
void initDebugFile(const HChar* debug_filename);

#endif
