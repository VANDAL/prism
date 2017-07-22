#ifndef PERFPT_H
#define PERFPT_H

#include "Core/Frontends.hpp"

auto startPerfPT(Args execArgs, Args feArgs, unsigned threads, sigil2::capabilities reqs)
    -> FrontendIfaceGenerator;
auto perfPTCapabilities() -> sigil2::capabilities;

#endif
