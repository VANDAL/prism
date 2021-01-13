#ifndef GENGRIND_H
#define GENGRIND_H

#include "Core/Frontends.hpp"
#include "Core/EventCapability.hpp"

auto startGengrind(Args execArgs,
                   Args feArgs,
                   unsigned threads,
                   prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator;
auto gengrindCapabilities() -> prism::capability::EvGenCaps;

#endif
