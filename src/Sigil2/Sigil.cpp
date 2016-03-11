#include <algorithm>

#include "spdlog.h"

#include "FrontEnds.hpp"
#include "Sigil.hpp"

void Sigil::generateEvents()
{
	start_frontend();
	mgr.finish();
}

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

void Sigil::registerToolParser(std::string toolname, std::function<void(int,char**)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].parse_args = handler;
}

void Sigil::registerToolFinish(std::string toolname, std::function<void(void)> handler)
{
	std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);
	backend_registry[toolname].finish = handler;
}


void Sigil::parseOptions(int argc, char *argv[])
{
	constexpr const char frontend[]="frontend";
	constexpr const char backend[]="backend";
	constexpr const char executable[]="executable";

	constexpr const char sigil2bin[] = "sigil2";
	constexpr const char frontend_usage[]=  "--frontend=FRONTEND [options]";
	constexpr const char backend_usage[]=   "--backend=BACKEND [options]";
	constexpr const char executable_usage[]="--executable=BINARY [options]";

	static auto parse_error_exit = [&](const std::string& msg)
	{
		spdlog::get("sigil2-console")->info() << "Error parsing arguments: " << msg;
		std::cout << "\nUSAGE:" << std::endl;
		std::cout << "    " << sigil2bin
			<< " " << frontend_usage
			<< " " << backend_usage
			<< " " << executable_usage << std::endl << std::endl;
		exit(1);
	};

	/* Sigil2 groups options together based on their position
	 * in order to pass the option group to the frontend
	 * instrumentation, the backend analysis, or the executable */
	/* Most parsers have limited or confusing support for 
	 * order of non-option arguments in relation to option 
	 * arguments, including getopt */

	/* Only allow long opts to avoid ambiguities.
	 * Additionally imposes the constraint that the frontend,
	 * backend, and executable cannot have any options that
	 * match */
	static class ArgGroup
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

	} arg_group;

	/* Pass through args to frontend/backend. */
	arg_group.addGroup(frontend);
	arg_group.addGroup(backend);
	arg_group.addGroup(executable);

	/* Parse loop */
	for (int optidx=1; optidx<argc; ++optidx)
	{
		const char* curr_arg = argv[optidx];

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

