#ifndef DRSIGIL_H
#define DRSIGIL_H

#include "Core/Frontends.hpp"

auto startDrSigil(Args execArgs, Args feArgs, unsigned threads, sigil2::capabilities reqs)
    -> FrontendIfaceGenerator;
auto drSigilCapabilities() -> sigil2::capabilities;

#endif
