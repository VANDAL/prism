#ifndef DRSIGIL_H
#define DRSIGIL_H

#include "Core/Frontends.hpp"

auto startDrSigil(Args execArgs, Args feArgs, unsigned threads, prism::capabilities reqs)
    -> FrontendIfaceGenerator;
auto drSigilCapabilities() -> prism::capabilities;

#endif
