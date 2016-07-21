#ifndef SGL_LOG_H
#define SGL_LOG_H

#include "spdlog.h"
/* TODO verify static initialization fiasco won't be a problem */

#define SGL_DEBUG

class Sigil;
class SigiLog
{
	friend Sigil;
	static std::shared_ptr<spdlog::logger> info_;
	static std::shared_ptr<spdlog::logger> warn_;
	static std::shared_ptr<spdlog::logger> error_;
	static std::shared_ptr<spdlog::logger> debug_;

public:
	static void info(const std::string &msg) { info_->info(msg); }
	static void warn(const std::string &msg) { warn_->warn(msg); }
	static void error(const std::string &msg) { error_->error(msg); }
	static void debug(const std::string &msg)
	{
#ifdef SGL_DEBUG
		debug_->debug(msg);
#endif
	}

	[[noreturn]] static void fatal(const std::string &msg)
	{
		error_->error(std::string("Fatal: ").append(msg));
		std::exit(EXIT_FAILURE);
	}
};

#endif
