#include <algorithm>

#include "SigiLog.hpp"

#include "InstrumentationIface.h"
#include "FrontEnds.hpp"
#include "Sigil.hpp"


////////////////////////////////////////////////////////////
// initialize logging
////////////////////////////////////////////////////////////
std::shared_ptr<spdlog::logger> SigiLog::info_ = nullptr;
std::shared_ptr<spdlog::logger> SigiLog::warn_ = nullptr;
std::shared_ptr<spdlog::logger> SigiLog::error_ = nullptr;

Sigil::Sigil()
{
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

	SigiLog::info_ = spdlog::stderr_logger_st("sigil2-console");
	SigiLog::info_->set_pattern(header+info+"  %v");

	SigiLog::warn_ = spdlog::stderr_logger_st("sigil2-warn");
	SigiLog::warn_->set_pattern(header+warn+"  %v");

	SigiLog::error_ = spdlog::stderr_logger_st("sigil2-err");
	SigiLog::error_->set_pattern(header+error+" %v");
}


////////////////////////////////////////////////////////////
// event generation loop
////////////////////////////////////////////////////////////
void Sigil::generateEvents()
{
	assert(start_frontend != nullptr);

	start_frontend();
	mgr.finish();
}


////////////////////////////////////////////////////////////
// instrumentation interface
////////////////////////////////////////////////////////////
void SGLnotifyMem(SglMemEv ev)
{
	Sigil::instance().addEvent(ev);
}


void SGLnotifyComp(SglCompEv ev)
{
	Sigil::instance().addEvent(ev);
}


void SGLnotifyCxt(SglCxtEv ev)
{
	Sigil::instance().addEvent(ev);
}


void SGLnotifySync(SglSyncEv ev)
{
	Sigil::instance().addEvent(ev);
}


/* XXX unimplemented */
//void SGLnotifyCF(SglCFEv ev)
//{
//}


////////////////////////////////////////////////////////////
// Event Handler Registration
////////////////////////////////////////////////////////////
void Sigil::registerEventHandler(std::string toolname, std::function<void(SglMemEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].mem_callback = handler;
}


void Sigil::registerEventHandler(std::string toolname, std::function<void(SglCompEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].comp_callback = handler;
}


void Sigil::registerEventHandler(std::string toolname, std::function<void(SglSyncEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].sync_callback = handler;
}


void Sigil::registerEventHandler(std::string toolname, std::function<void(SglCxtEv)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].cxt_callback = handler;
}


void Sigil::registerToolParser(std::string toolname, std::function<void(std::vector<std::string>)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].parse_args = handler;
}


void Sigil::registerToolFinish(std::string toolname, std::function<void(void)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].finish = handler;
}



////////////////////////////////////////////////////////////
// Argument Parsing
////////////////////////////////////////////////////////////
namespace
{

constexpr const char frontend[]="frontend";
constexpr const char backend[]="backend";
constexpr const char executable[]="executable";

constexpr const char sigil2bin[] = "sigil2";
constexpr const char frontend_usage[]=  "--frontend=FRONTEND [options]";
constexpr const char backend_usage[]=   "--backend=BACKEND [options]";
constexpr const char executable_usage[]="--executable=BINARY [options]";


[[noreturn]] void parse_error_exit(const std::string& msg)
{
	SigiLog::error("Error parsing arguments: " + msg);

	std::cout << "\nUSAGE:" << std::endl;
	std::cout << "    " << sigil2bin
		<< " " << frontend_usage
		<< " " << backend_usage
		<< " " << executable_usage << std::endl << std::endl;

	exit(EXIT_FAILURE);
}


/* Sigil2 groups options together based on their position
 * in order to pass the option group to the frontend
 * instrumentation, the backend analysis, or the executable
 *
 * Most parsers have limited or confusing support for 
 * order of non-option arguments in relation to option 
 * arguments, including getopt
 *
 * Only allow long opts to avoid ambiguities.
 * Additionally imposes the constraint that the frontend,
 * backend, and executable cannot have any options that match */
class ArgGroup
{
	using Args = std::vector<std::string>;

	/* long opt -> args */
	std::map<std::string, std::vector<std::string>> args;
	std::vector<std::string> empty;
	std::string prev_opt;

public:
	/* Add a long option to group args */
	void addGroup(const std::string &group)
	{
		if (group.empty() == true) return;
		args.emplace(group, std::vector<std::string>());
	}

	/* Check an argv[] to see if it's been added as an
	 * arg group. If it is a validly formed arg group,
	 * begin grouping consecutive options under this group
	 * and return true; otherwise return false.
	 *
	 * long_opt is to be in the form: "--long_opt=argument" */
	bool tryGroup(const std::string &arg)
	{
		/* only long opts valid */
		if (arg.substr(0,2).compare("--") != 0)
		{
			return false;
		}

		std::string rem(arg.substr(2));
		auto eqidx = rem.find('=');

		/* was this added? */
		if (args.find(rem.substr(0, eqidx)) == args.cend())
		{
			return false;
		}

		/* a valid arg group requires '=argument' */
		if (eqidx == std::string::npos || eqidx == rem.size()-1)
		{
			parse_error_exit(std::string(arg).append(" missing argument"));
		}

		/* duplicate option groups not allowed */
		prev_opt = rem.substr(0, eqidx);
		if (args.at(prev_opt).empty() == false)
		{
			parse_error_exit(std::string(arg).append(" is duplicate option"));
		}

		/* initialize the group of args with this first argument */
		args.at(prev_opt).push_back(rem.substr(eqidx+1));

		return true;
	}

	void addArg(const std::string &arg)
	{
		if (arg.empty() == true) return;

		/* the first argument must be an arg group */
		if (prev_opt.empty() == true)
		{
			parse_error_exit(std::string(arg).append(" is not valid here"));
		}

		args.at(prev_opt).push_back(arg);
	}

	Args& operator[](const std::string &group)
	{
		if (args.find(group) == args.cend())
		{
			return empty;
		}
		else
		{
			return args.at(group);
		}
	}
};

}; //end namespace


void Sigil::parseOptions(int argc, char *argv[])
{
	ArgGroup arg_group;

	/* Pass through args to frontend/backend. */
	arg_group.addGroup(frontend);
	arg_group.addGroup(backend);
	arg_group.addGroup(executable);

	/* Parse loop */
	for (int optidx=1; optidx<argc; ++optidx)
	{
		const char *curr_arg = argv[optidx];

		if (arg_group.tryGroup(curr_arg) == false)
		{
			arg_group.addArg(curr_arg);
		}
	}

	if (arg_group[frontend].empty() == true
			|| arg_group[backend].empty() == true
			|| arg_group[executable].empty() == true)
	{
		parse_error_exit("missing required arguments");
	}

	/* check valid frontends */
	std::string frontend_name = arg_group[frontend][0];
	std::transform(frontend_name.begin(), frontend_name.end(), frontend_name.begin(), ::tolower);
	if (frontend_name.compare("valgrind") == 0)
	{
		auto start = arg_group[frontend].cbegin()+1;
		auto end = arg_group[frontend].cend();
		auto frontend_args = std::vector<std::string>(start, end);

		start_frontend = std::bind(
				sgl::frontendSigrind,
				arg_group[executable],
				frontend_args);
	}
	else
	{
		parse_error_exit(" invalid frontend");
	}

	/* check valid backends */
	std::string backend_name = arg_group[backend][0];
	std::transform(backend_name.begin(), backend_name.end(), backend_name.begin(), ::tolower);
	if (backend_registry.find(backend_name) != backend_registry.cend())
	{
		/* Set up event callbacks. 
		 * Only one backend supported for now */
		Backend &registered = backend_registry[backend_name];
		if (registered.mem_callback  != nullptr) mgr.addObserver(registered.mem_callback);
		if (registered.comp_callback != nullptr) mgr.addObserver(registered.comp_callback);
		if (registered.sync_callback != nullptr) mgr.addObserver(registered.sync_callback);
		if (registered.cxt_callback  != nullptr) mgr.addObserver(registered.cxt_callback);
		if (registered.finish != nullptr) mgr.addCleanup(registered.finish);
		if (registered.parse_args != nullptr) registered.parse_args({arg_group[backend].cbegin()+1,arg_group[backend].cend()});
	}
	else
	{
		std::string backend_error(" invalid backend argument ");
		backend_error.append(backend_name).append("\n");

		backend_error.append("\tAvailable backends: ");
		for (auto p : backend_registry)
		{
			backend_error.append("\n\t").append(p.first);
		}

		parse_error_exit(backend_error);
	}
}
