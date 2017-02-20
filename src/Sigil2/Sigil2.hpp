#ifndef SIGIL2_EVENTMANAGER_H
#define SIGIL2_EVENTMANAGER_H

#include "Sigil2Config.hpp"

auto startSigil2(const Sigil2Config& config) -> int;

/* Thread for moving tasks from frontend -> backend */
auto consumeEvents(BackendGenerator createBEIface, FrontendIfaceGenerator createFEIface) -> void;

#endif
