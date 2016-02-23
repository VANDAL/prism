#ifndef SIGIL_H
#define SIGIL_H

#include <map>

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
	void registerEventHandler(std::string &toolname, std::function<void(SglMemEv)> handler);
	void registerEventHandler(std::string &toolname, std::function<void(SglCompEv)> handler);
	void registerEventHandler(std::string &toolname, std::function<void(SglSyncEv)> handler);
	void registerEventHandler(std::string &toolname, std::function<void(SglCxtEv)> handler);
	void registerToolFinish(std::string &toolname, std::function<void(void)> handler);
	void registerToolParser(std::string &toolname, std::function<void(int,char**)> handler);

	template <typename T>
	void addEvent(const T &ev)
	{
		mgr.addEvent(ev);
	}

	void parseOptions(int argc, char *argv[]);
	void generateEvents();

private:
	Sigil(){}

	sgl::EventManager mgr;
	std::map<std::string, Backend> backend_registry;
	std::function<void()> start_frontend;
};

#endif
