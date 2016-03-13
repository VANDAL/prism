#ifndef SIGIL_H
#define SIGIL_H

#include <map>

#include "spdlog.h"

#include "Primitive.h"
#include "EventManager.hpp"

class Sigil
{
	struct Backend
	{
		/* A backend can process events, process arguments, print a usage,
		 * or do none of these things */
		std::function<void(SglMemEv)> mem_callback;
		std::function<void(SglCompEv)> comp_callback;
		std::function<void(SglSyncEv)> sync_callback;
		std::function<void(SglCxtEv)> cxt_callback;
		std::function<void(void)> finish;
		
		/* XXX unimplemented */
		std::function<void(int,char**)> parse_args;

		Backend()
		{
			mem_callback = nullptr;
			comp_callback = nullptr;
			sync_callback = nullptr;
			cxt_callback = nullptr;
			finish = nullptr;
			parse_args = nullptr;
		}
	};

public:
	static Sigil &instance()
	{
		static Sigil singleton;
		return singleton;
	}

	void registerEventHandler(std::string toolname, std::function<void(SglMemEv)> handler);
	void registerEventHandler(std::string toolname, std::function<void(SglCompEv)> handler);
	void registerEventHandler(std::string toolname, std::function<void(SglSyncEv)> handler);
	void registerEventHandler(std::string toolname, std::function<void(SglCxtEv)> handler);
	void registerToolFinish(std::string toolname, std::function<void(void)> handler);
	void registerToolParser(std::string toolname, std::function<void(int,char**)> handler);

	template <typename T>
	void addEvent(const T &ev)
	{
		mgr.addEvent(ev);
	}

	void parseOptions(int argc, char *argv[]);
	void generateEvents();

private:
	Sigil()
	{
		/* Set up Sigil2 logging.
		 * TODO this needs to be cleaned up;
		 * an access to spdlog via spdlog::get with an invalid
		 * descriptor will fail with a segfault and may be confusing 
		 * to debug. Perhaps make a global variable somewhere... */
		std::map<std::string,std::string> ANSIcolors_fg =
		{
			{"black", "\033[30m"},
			{"red", "\033[31m"},
			{"green", "\033[32m"},
			{"yellow", "\033[33m"},
			{"blue", "\033[34m"},
			{"magenta", "\033[35m"},
			{"cyan", "\033[36m"},
			{"white", "\033[37m"},
			{"end", "\033[0m"}
		};

		auto color = [&ANSIcolors_fg](const char* text, const char* color)
		{
			std::string ret(text);
			if (isatty(fileno(stdout))) ret = std::string(ANSIcolors_fg[color]).append(text).append(ANSIcolors_fg["end"]);
			return ret;
		};

		std::string header = "[Sigil2]";
		std::string info = "[" + color("INFO", "blue") + "]";
		std::string warn = "[" + color("WARN", "yellow") + "]";
		std::string error = "[" + color("ERROR", "red") + "]";

		spdlog::set_sync_mode();

		auto out_logger = spdlog::stderr_logger_st("sigil2-console");
		out_logger->set_pattern(header+info+"  %v");

		auto warn_logger = spdlog::stderr_logger_st("sigil2-warn");
		warn_logger->set_pattern(header+warn+"  %v");

		auto err_logger = spdlog::stderr_logger_st("sigil2-err");
		err_logger->set_pattern(header+error+" %v");
	}

	sgl::EventManager mgr;
	std::map<std::string, Backend> backend_registry;
	std::function<void()> start_frontend;
};

#endif
