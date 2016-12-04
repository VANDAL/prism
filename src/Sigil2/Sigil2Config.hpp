#ifndef SIGIL2_CONFIG_H
#define SIGIL2_CONFIG_H

#include "Backends.hpp"
#include "Frontends.hpp"
#include "Sigil2Parser.hpp"

class Sigil2Config
{
  public:
    Sigil2Config()  = default;
    ~Sigil2Config() = default;

    /* configuration */
    auto registerBackend(ToolName name, Backend be)   -> Sigil2Config&;
    auto registerFrontend(ToolName name, Frontend fe) -> Sigil2Config&;
    auto parseCommandLine(int argc, char* argv[])     -> Sigil2Config&;

    /* accessors */
    auto threads()  const -> int      { return _threads;  }
    auto backend()  const -> Backend  { return _backend;  }
    auto frontend() const -> Frontend { return _frontend; }

  private:
    BackendFactory beFactory;
    FrontendFactory feFactory;

    int _threads;
    Backend _backend;
    Frontend _frontend;
};

#endif
