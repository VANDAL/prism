#ifndef SGL_AVAILABLE_FRONTENDS_H
#define SGL_AVAILABLE_FRONTENDS_H

#include "Sigil2/Frontends.hpp"
auto startSigrind(FrontendStarterArgs args) -> FrontendIfaceGenerator;
auto startDrSigil(FrontendStarterArgs args) -> FrontendIfaceGenerator;

#endif
