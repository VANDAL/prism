#include <algorithm>

#include "spdlog.h"
#include "optionparser.h"

#include "FrontEnds.hpp"
#include "Sigil.hpp"

void Sigil::registerEventHandler(std::string &toolname, std::function<void(SglMemEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].mem_callback = handler;
}

void Sigil::registerEventHandler(std::string &toolname, std::function<void(SglCompEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].comp_callback = handler;
}

void Sigil::registerEventHandler(std::string &toolname, std::function<void(SglSyncEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].sync_callback = handler;
}

void Sigil::registerEventHandler(std::string &toolname, std::function<void(SglCxtEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].cxt_callback = handler;
}

void Sigil::registerToolParser(std::string &toolname, std::function<void(int,char**)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].parse_args = handler;
}

void Sigil::registerToolFinish(std::string &toolname, std::function<void(void)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].finish = handler;
}

namespace
{
struct Arg : public option::Arg
{
	static option::ArgStatus Unknown(const option::Option& option, bool msg)
	{
		if (msg == true) 
		{
			spdlog::get("sigil2-console")->info() << "Unknown option '" << option.name << "'";
		}

		return option::ARG_ILLEGAL;
	}
	
	static option::ArgStatus Required(const option::Option& option, bool msg)
	{
		if (option.arg != 0 && *option.arg != 0 && option.name[option.namelen] != 0)
		{
			return option::ARG_OK;
		}
	
		if (msg == true) 
		{
			spdlog::get("sigil2-console")->info() << "Option '" << option.name << "' requires an argument"; 
			spdlog::get("sigil2-console")->info() << "Arguments are attached with '='";
		}

		return option::ARG_ILLEGAL;
	}
};

enum optionIndex 
{ 
	UNKNOWN, 
	HELP, 
	FRONTEND, 
	BACKEND, 
	FRONTEND_ARGS, 
	BACKEND_ARGS, 
	EXEC_COMMAND 
};

const option::Descriptor usage[] =
{
	{UNKNOWN       ,0, ""  , ""         ,  Arg::Unknown     , 
		"USAGE: sigil2 --frontend=TOOL [--frontend-args=\"options\"]\n"
		"              --backend=TOOL [--backend-args=\"options\"]\n"
		"              --exec=\"EXECUTABLE [options]\"\n\n"
																"Options:" },
	{HELP          ,0, ""  , "help"     , option::Arg::None     , "  --help, -h   Print usage."},
	{FRONTEND      ,0, ""  , "frontend" , Arg::Required         , "  --frontend=TOOL\n        Where TOOL can be:\n        valgrind\n"},
	{BACKEND       ,0, ""  , "backend"  , Arg::Required         , "  --backend=TOOL\n        Where TOOL can be:\n        valgrind\n"},
	{FRONTEND_ARGS ,0, ""  , "frontend-args" , Arg::Required    , "  --frontend-args=\"options\"\n        Where \"options\" are options for the frontend\n"},
	{BACKEND_ARGS  ,0, ""  , "backend-args"  , Arg::Required    , "  --backend-args=\"[tool options]\""},
	{EXEC_COMMAND  ,0, ""  , "exec"     , Arg::Required         , "  --exec=          \"[executable] [options]\""},
	{0             ,0, 0   , 0          , Arg::Optional         , 0 }
};
};

void Sigil::parseOptions(int argc, char *argv[])
{
	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present

	option::Stats stats(usage, argc, argv);
	auto options = new option::Option[stats.options_max];
	auto buffer = new option::Option[stats.buffer_max];
	option::Parser parse(true, usage, argc, argv, options, buffer);

	if (parse.error())
	{
		spdlog::get("sigil2-console")->info() << "Error parsing arguments...exiting";
		exit(1);
	}

	/* options are only passed with -ab or --long for flags
	 * or -a=b --opt=arg for arguments */
	if (parse.nonOptionsCount() > 0)
	{
		spdlog::get("sigil2-console")->info() << "Encountered unhandled arguments";
		spdlog::get("sigil2-console")->info() << "Error parsing arguments...exiting";
		exit(1);
	}

	if (options[HELP] || argc == 0)
	{
		int columns = getenv("COLUMNS")? atoi(getenv("COLUMNS")) : 80;
		option::printUsage(fwrite, stdout, usage, columns);
		exit(1);
	}

	if (options[EXEC_COMMAND] == nullptr || options[FRONTEND] == nullptr || options[BACKEND] == nullptr)
	{
		spdlog::get("sigil2-console")->info() << "Missing required arguments";
		spdlog::get("sigil2-console")->info() << "Error parsing arguments...exiting";
		exit(1);
	}

	std::string exec = options[EXEC_COMMAND].arg;

	/* check valid frontends */
	std::string frontend_name = options[FRONTEND].arg;
	std::transform(frontend_name.begin(), frontend_name.end(), frontend_name.begin(), ::tolower);
	if (frontend_name.compare("valgrind") == 0)
	{
		std::string frontend_args;
		if (options[FRONTEND_ARGS].arg != nullptr) frontend_args = options[FRONTEND_ARGS].arg;

		start_frontend = std::bind(sgl::frontendSigrind, exec, frontend_args); 
	}
	else
	{
		spdlog::get("sigil2-console")->info() << "Invalid frontend argument: " << frontend_name;
		//TODO list available frontends
		exit(1);
	}

	/* check valid backends */
	std::string backend_name = options[BACKEND].arg;
	std::transform(backend_name.begin(), backend_name.end(), backend_name.begin(), ::tolower);
	if (backend_registry.find(backend_name) != backend_registry.cend())
	{
		/* Set up event callbacks. 
		 * Only one backend supported for now */
		Backend &backend = backend_registry[backend_name];
		if (backend.mem_callback  != nullptr) mgr.addObserver(backend.mem_callback);
		if (backend.comp_callback != nullptr) mgr.addObserver(backend.comp_callback);
		if (backend.sync_callback != nullptr) mgr.addObserver(backend.sync_callback);
		if (backend.cxt_callback  != nullptr) mgr.addObserver(backend.cxt_callback);
		if (backend.finish  != nullptr) mgr.addCleanup(backend.finish);
	}
	else
	{
		spdlog::get("sigil2-console")->info() << "Invalid backend argument: " << backend_name;

		std::string available_backends("Available backends: ");
		for (auto p : backend_registry)
		{
			available_backends += "\n\t" + p.first;
		}
		spdlog::get("sigil2-console")->info() << available_backends;

		exit(1);
	}
}

void Sigil::generateEvents()
{
	start_frontend();
	mgr.finish();
}
