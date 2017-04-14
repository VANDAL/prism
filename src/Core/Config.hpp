#ifndef SIGIL2_CONFIG_H
#define SIGIL2_CONFIG_H

#include "Backends.hpp"
#include "Frontends.hpp"
#include "Parser.hpp"

namespace sigil2
{

class Config
{
  public:
    auto registerBackend(ToolName name, Backend be)             -> Config&;
    auto registerFrontend(ToolName name, FrontendStarter start) -> Config&;
    auto parseCommandLine(int argc, char* argv[])               -> Config&;
    /* configuration */

    auto threads()       const -> int                    { return _threads;  }
    auto backend()       const -> Backend                { return _backend;  }
    auto startFrontend() const -> FrontendStarterWrapper { return _startFrontend; }
    /* accessors */

  private:
    BackendFactory beFactory;
    FrontendFactory feFactory;

    int _threads;
    Backend _backend;
    FrontendStarterWrapper _startFrontend;
};

}; //end namespace sigil2

#endif
