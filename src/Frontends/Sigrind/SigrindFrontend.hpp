#ifndef SIGRIND_H
#define SIGRIND_H

#include "Core/Frontends.hpp"

auto startSigrind(Args execArgs, Args feArgs, unsigned threads, sigil2::capabilities reqs)
    -> FrontendIfaceGenerator;
auto sigrindCapabilities() -> sigil2::capabilities;

#endif
