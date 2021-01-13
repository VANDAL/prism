#ifndef DRSIGIL_H
#define DRSIGIL_H

#include "Core/Frontends.hpp"
#include "Core/EventCapability.hpp"

auto startDrSigil(Args execArgs,
                  Args feArgs,
                  unsigned threads,
                  prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator;
auto drSigilCapabilities() -> prism::capability::EvGenCaps;

#endif
