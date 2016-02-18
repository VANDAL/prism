#ifndef SGL_OPTIONPARSER_H
#define SGL_OPTIONPARSER_H

#include <functional>
#include "spdlog.h"

namespace sgl
{
class OptionParser
{
	bool registerBackendArgument(const std::string& backend);
	bool registerFrontendArgument(const std::string& frontend, const std::string& exec);

	std::shared_ptr<spdlog::logger> stdout_logger;

public:
	std::function<void()> register_backend = nullptr;
	std::function<void()> start_frontend = nullptr;
	OptionParser(int argc, char* argv[]);

private:
	void parse(int argc, char* argv[]);
};
}; //end namespace sgl

#endif
