#ifndef GENGRIND_H
#define GENGRIND_H

#include "Core/Frontends.hpp"

auto startGengrind(Args execArgs, Args feArgs, unsigned threads, prism::capabilities reqs)
    -> FrontendIfaceGenerator;
auto gengrindCapabilities() -> prism::capabilities;

#endif
