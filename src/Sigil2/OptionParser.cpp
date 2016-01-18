#include "OptionParser.hpp"
#include "Plugins.cpp"
#include "FrontEnds.hpp"

#include <getopt.h>
#include <dlfcn.h>
#include <iostream>

extern char* optarg;

namespace sgl
{
namespace
{
struct option long_options[] =
{
	{"help",     no_argument,       0, 'h' },
	{"backend",  required_argument, 0, 'b' },
	{"frontend", required_argument, 0, 'f' },
	{0,          0,                 0,  0  }
};
}; //end namespace

OptionParser::OptionParser(int argc, char* argv[])
{
	int c;
	int option_index = 0;
	while ( (c = getopt_long(argc,argv,"hb:f:", long_options, &option_index) ) != 0 )
	{
		if (c == -1)
		{
			break;
		}

		switch (c)
		{
		case 0:
			if ( long_options[option_index].flag != 0 )
				break;
		case 'h':
			break;
		case 'b':
			if ( registerBackendArgument(optarg) == false )
			{
				std::cerr << "Error initializing backend" << std::endl;
				exit( EXIT_FAILURE );
			}
			break;
		case 'f':
			if ( registerFrontendArgument(optarg) == false )
			{
				std::cerr << "Error initializing frontend" << std::endl;
				exit( EXIT_FAILURE );
			}
			break;
		}
	}

	if ( start_backend == nullptr || start_frontend == nullptr )
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
		std::cerr << "Please report this issue at:" << std::endl;
		std::cerr << "\thttps://github.com/mdlui/Sigil2" << std::endl;
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

	start_backend = ((void (*)(void))(fptr));
	return true;
}

bool OptionParser::registerFrontendArgument(const std::string& frontend)
{
	if (frontend.compare("vg") == 0)
	{
		start_frontend = std::bind(&sigrind_listener, 0, nullptr);
		return true;
	}
	else
	{
		return false;
	}
}

}; //end namespace sgl
