#ifndef PERFPT_H
#define PERFPT_H

#include "Core/Frontends.hpp"

auto startPerfPT(Args execArgs, Args feArgs, unsigned threads, prism::capabilities reqs)
    -> FrontendIfaceGenerator;
auto perfPTCapabilities() -> prism::capabilities;

#endif
