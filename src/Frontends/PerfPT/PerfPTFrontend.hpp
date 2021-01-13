#ifndef PERFPT_H
#define PERFPT_H

#include "Core/Frontends.hpp"
#include "Core/EventCapability.hpp"

auto startPerfPT(Args execArgs,
                 Args feArgs,
                 unsigned threads,
                 prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator;
auto perfPTCapabilities() -> prism::capability::EvGenCaps;

#endif
