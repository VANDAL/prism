#ifndef SGL_LOGGER_H
#define SGL_LOGGER_H

namespace sgl
{
/* The log file must be created before it can be used */
void logCreateFile(const char* const filename);

/* Write a string to the file previously set up */
void logToFile(const char* const filename, const char* const msg);
}

#endif
