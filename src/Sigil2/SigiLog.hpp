#ifndef SGL_LOG_H
#define SGL_LOG_H

#include "spdlog.h"
/* TODO verify static initialization fiasco won't be a problem */

class Sigil;
class SigiLog
{
	friend Sigil;
	static std::shared_ptr<spdlog::logger> info_;
	static std::shared_ptr<spdlog::logger> warn_;
	static std::shared_ptr<spdlog::logger> error_;

public:
	static void info(const std::string &msg) { info_->info(msg); }
	static void warn(const std::string &msg) { warn_->warn(msg); }
	static void error(const std::string &msg) { error_->error(msg); }
};

#endif
