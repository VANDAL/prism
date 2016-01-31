#include "OptionParser.hpp"
#include "Plugins.cpp"
#include "FrontEnds.hpp"

#include <getopt.h>
#include <dlfcn.h>
#include <cstring>
#include <iostream>

extern char* optarg;

namespace sgl
{
namespace
{
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
		std::cerr << "Error initializing frontend" << std::endl;
		exit( EXIT_FAILURE );
	}

	if ( register_backend == nullptr || start_frontend == nullptr )
	{
		std::cerr << "Error | missing options" << std::endl;
		exit( EXIT_FAILURE );
	}
}

bool OptionParser::registerBackendArgument(const std::string& backend)
{
	std::string prefix(SGL_STRINGIFYX(REGISTER_PREFIX));

	void *hndl = dlopen(NULL, RTLD_LAZY);
	if ( hndl == nullptr )
	{
		std::cerr << "dlopen failed: " << dlerror()	<< std::endl;
		return false;
	}

	std::string func_name = prefix + backend;
	void* fptr = dlsym(hndl, func_name.c_str());
	if ( fptr == nullptr )
	{
		std::cerr << "Could not find a matching backend registry" << std::endl;
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
		start_frontend = std::bind(&frontendSigrind, exec, std::string("./vg-bin/bin"));
		return true;
	}
	else
	{
		return false;
	}
}

}; //end namespace sgl
