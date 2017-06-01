#ifndef SIGIL2_CONFIG_H
#define SIGIL2_CONFIG_H

#include "Backends.hpp"
#include "Frontends.hpp"
#include "Parser.hpp"
#include <cassert>

namespace sigil2
{

class Config
{
  public:
    auto registerBackend(ToolName name, Backend be)             -> Config&;
    auto registerFrontend(ToolName name, FrontendStarter start) -> Config&;
    auto parseCommandLine(int argc, char* argv[])               -> Config&;
    /* configuration */

    auto timed()         const -> bool                   { return _timed; }
    auto threads()       const -> int                    { return _threads; }
    auto backend()       const -> Backend                { return _backend; }
    auto startFrontend() const -> FrontendStarterWrapper { return _startFrontend; }
    auto threadsPrintable()    const -> std::string { assert(parsed); return std::to_string(_threads); }
    auto backendPrintable()    const -> std::string { assert(parsed); return backendName; }
    auto frontendPrintable()   const -> std::string { assert(parsed); return frontendName; }
    auto executablePrintable() const -> std::string { assert(parsed); return executableName; }
    /* accessors */

  private:
    BackendFactory beFactory;
    FrontendFactory feFactory;

    bool _timed;
    int _threads;
    Backend _backend;
    FrontendStarterWrapper _startFrontend;

    std::string backendName;
    std::string frontendName;
    std::string executableName;

    bool parsed{false};
};

}; //end namespace sigil2

#endif
