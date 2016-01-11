#ifndef SGL_LOG_H
#define SGL_LOG_H

#include "spdlog/include/spdlog/spdlog.h"
#include <string>

namespace sgl
{
	/* The log file must be created before it can be used */
	void logCreateFile(const char* const filename);

	/* Write a string to the file previously set up */
	void logToFile(const char* const filename, const char* const msg);
}

#endif
