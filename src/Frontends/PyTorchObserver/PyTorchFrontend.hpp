#ifndef PYTORCHOBSERVER_H
#define PYTORCHOBSERVER_H

#include "Core/Frontends.hpp"
#include "Core/EventCapability.hpp"

auto startPyTorchCaffe2(
    Args execArgs,
    Args feArgs,
    unsigned threads,
    prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator;

auto PyTorchCapabilities() -> prism::capability::EvGenCaps;

#endif
