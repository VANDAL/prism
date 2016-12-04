#ifndef SIGIL2_EVENTMANAGER_H
#define SIGIL2_EVENTMANAGER_H

#include "Sigil2Config.hpp"

auto startSigil2(const Sigil2Config& config) -> int;

/* Thread for moving tasks from frontend -> backend */
auto consumeEvents(BackendGenerator be, int idx,
                   FrontendBufferAcquire acq,
                   FrontendBufferRelease rel,
                   FrontendReady ready) -> void;

#endif
