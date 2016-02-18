#include "OptionParser.hpp"
#include "Plugins.cpp"
#include "FrontEnds.hpp"

#include <getopt.h>
#include <dlfcn.h>
#include <cstring>
#include <unistd.h>

// FIXME option parsing errors should not be 'runtime_errors'
// Do something more useful with unexpected options

extern char* optarg;

namespace sgl
{
namespace
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

struct option long_options[] =
{
	{"help",          no_argument,       nullptr, 'h' },
	{"backend",       required_argument, nullptr, 'b' },
	{"frontend",      required_argument, nullptr, 'f' },
	{"exec",          required_argument, nullptr,  0  },
	{nullptr,         0,                 nullptr,  0  }
};
}; //end namespace

OptionParser::OptionParser(int argc, char* argv[])
{
	stdout_logger = spdlog::stdout_logger_st("sigil2-console");

	std::string header = "[Sigil2]";
	if (isatty(fileno(stdout))) header = "[" + ANSIcolors_fg["red"] + "Sigil2" + ANSIcolors_fg["end"] + "]";
	stdout_logger->set_pattern(header+" %v");

	try
	{
		parse(argc,argv);
	}
	catch (std::runtime_error &e)
	{
		std::terminate();
	}
	catch (std::exception &e)
	{
		std::terminate();
	}
}
	
void OptionParser::parse(int argc, char* argv[])
{
	std::string exec;
	std::string frontend;

	int c, option_index;
	while ( (c = getopt_long(argc,argv,"hb:f:", long_options, &option_index) ) != -1 )
	{
		switch (c)
		{
		case 0:
			exec = optarg;
			break;
		case 'h':
			break;
		case 'b':
			if ( registerBackendArgument(optarg) == false )
			{
				throw std::runtime_error("Error initializing backend");
			}
			break;
		case 'f':
			frontend = optarg;
			break;
		case '?':
			break;
		}
	}

	if ( registerFrontendArgument(frontend, exec) == false )
	{
		throw std::runtime_error("Error initializing frontend");
	}

	if ( register_backend == nullptr || start_frontend == nullptr )
	{
		throw std::runtime_error("Error | missing options");
	}
}

bool OptionParser::registerBackendArgument(const std::string& backend)
{
	std::string prefix(SGL_STRINGIFYX(REGISTER_PREFIX));

	void *hndl = dlopen(NULL, RTLD_LAZY);
	if ( hndl == nullptr )
	{
		stdout_logger->info("dlopen failed: ") << dlerror();
		return false;
	}

	std::string func_name = prefix + backend;
	void* fptr = dlsym(hndl, func_name.c_str());
	if ( fptr == nullptr )
	{
		stdout_logger->info("Could not find a matching backend registry");
		return false;
		//TODO output a list of available backends
	}

	register_backend = ((void (*)(void))(fptr));
	return true;
}

bool OptionParser::registerFrontendArgument(const std::string& frontend, const std::string& exec)
{
	if (frontend.compare("vg") == 0)
	{
		//TODO option or env var for valgrind/sigrind install directory
		
		char* tmp_path = std::getenv("TMPDIR");
		if (tmp_path == nullptr)
		{
			stdout_logger->info("TMPDIR not detected, defaulting to /tmp");
			tmp_path = strdup("/tmp");
		}

		start_frontend = std::bind(&frontendSigrind, 
				exec, 
				std::string("./vg-bin/bin"), 
				std::string(tmp_path)
				);

		return true;
	}
	else
	{
		return false;
	}
}

}; //end namespace sgl
