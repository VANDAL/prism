#ifndef PRISM_COREEVENTLOOP_H
#define PRISM_COREEVENTLOOP_H

#include "Config.hpp"
#include "EventIface.hpp"

namespace prism {
auto startPrism(const Config& config) -> int;
auto flushToBackend(BackendIface &be, EventStreamParserConfig &evStreamParserCfg, const unsigned char* buf) -> size_t; 
auto consumeEvents(BackendIfaceGenerator createBEIface, FrontendIfaceGenerator createFEIface, EventStreamParserConfig evStreamParserCfg) -> void;
} // end namespace prism

#endif
