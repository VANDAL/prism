#ifndef SIGIL_H
#define SIGIL_H

#include <map>

#include "Primitive.h"
#include "EventManager.hpp"

class Sigil
{
public:
	using ToolName = std::string;
	using Args = std::vector<std::string>;
	using BackendArgparser = std::function<void(Args)>;
	using BackendRegistration = std::function<void(void)>;

	static Sigil &instance()
	{
		static Sigil singleton;
		return singleton;
	}

	/* main interface */
	void parseOptions(int argc, char *argv[]);
	void generateEvents();

	/* event generation interface */
	template <typename T>
	void addEvent(const T &ev)
	{
		mgr.addEvent(ev);
	}

	/* static plugin interface */
	void registerBackend(ToolName name, BackendRegistration registration);
	template <typename Func>
	void registerEventHandler(Func handler) {mgr.addObserver(handler);}
	void registerToolFinish(std::function<void(void)> handler) {mgr.addCleanup(handler);}
	void registerToolParser(BackendArgparser parser) {backend_parser = parser;}

private:
	Sigil();

	sgl::EventManager mgr;
	std::map<ToolName, BackendRegistration> backend_registry;
	BackendArgparser backend_parser;
	std::function<void(void)> start_frontend;
};

#endif
