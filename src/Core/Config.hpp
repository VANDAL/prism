#ifndef PRISM_CONFIG_H
#define PRISM_CONFIG_H

#include "Backends.hpp"
#include "Frontends.hpp"
#include "Parser.hpp"
#include <cassert>

namespace prism
{

class Config
{
  public:
    auto registerBackend(ToolName name,
                         BackendIfaceGenerator beGenerator,
                         BackendParser beParser,
                         BackendFinish beFinish,
                         prism::capabilities beRequirements) -> Config&;
    auto registerFrontend(ToolName name, Frontend fe) -> Config&;
    auto parseCommandLine(int argc, char* argv[]) -> Config&;
    /* configuration */

    auto timed() const { return _timed;   }
    auto threads() const { return _threads; }
    auto backend() const { return _backend; }
    auto frontend() const { return _frontend; }
    auto startFrontend() const { return _startFrontend; }
    auto threadsPrintable() const { assert(parsed); return std::to_string(_threads); }
    auto backendPrintable() const { assert(parsed); return backendName; }
    auto frontendPrintable() const { assert(parsed); return frontendName; }
    auto executablePrintable() const { assert(parsed); return executableName; }
    /* accessors */

  private:
    BackendFactory beFactory;
    FrontendFactory feFactory;

    bool _timed;
    int _threads;
    Backend _backend;
    Frontend _frontend;
    FrontendStarterWrapper _startFrontend;

    std::string backendName;
    std::string frontendName;
    std::string executableName;

    bool parsed{false};
};

}; //end namespace prism

#endif
