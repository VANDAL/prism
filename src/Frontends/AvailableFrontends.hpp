#ifndef SGL_AVAILABLE_FRONTENDS_H
#define SGL_AVAILABLE_FRONTENDS_H

#include "Core/Frontends.hpp"

auto startSigrind(FrontendStarterArgs args) -> FrontendIfaceGenerator;
auto startDrSigil(FrontendStarterArgs args) -> FrontendIfaceGenerator;
auto startPerfPT(FrontendStarterArgs args) -> FrontendIfaceGenerator;

#endif
