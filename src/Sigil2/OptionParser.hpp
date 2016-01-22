#ifndef SGL_OPTIONPARSER_H
#define SGL_OPTIONPARSER_H

#include <functional>

namespace sgl
{
class OptionParser
{
	bool registerBackendArgument(const std::string& backend);
	bool registerFrontendArgument(const std::string& frontend, const std::string& exec);

public:
	std::function<void()> start_backend = nullptr;
	std::function<void()> start_frontend = nullptr;
	OptionParser(int argc, char* argv[]);
};
}; //end namespace sgl

#endif
