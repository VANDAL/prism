#include "Frontends.hpp"
#include "SigiLog.hpp"
#include <algorithm>

auto FrontendFactory::create(ToolName name, FrontendStarterArgs args) const -> Frontend
{
    /* default */
    if (name.empty() == true)
        name = "valgrind";

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (exists(name) == true)
    {
        auto frontend = registry.find(name)->second;
        frontend.args = args;
        return frontend;
    }
    else
    {
        std::string error(" invalid frontend argument ");
        error.append(name + "\n");
        error.append("\tAvailable frontends: ");

        for (auto name : available())
            error.append("\n\t").append(name);

        SigiLog::fatal(error);
    }
}


auto FrontendFactory::add(ToolName name, FrontendStarter start) -> void
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    auto p = registry.emplace(name, Frontend());
    p.first->second.start = start;
}


auto FrontendFactory::add(ToolName name, FrontendBufferAcquire acq) -> void
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    auto p = registry.emplace(name, Frontend());
    p.first->second.acq = acq;
}


auto FrontendFactory::add(ToolName name, FrontendBufferRelease rel) -> void
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    auto p = registry.emplace(name, Frontend());
    p.first->second.rel = rel;
}


auto FrontendFactory::add(ToolName name, FrontendReady ready) -> void
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    auto p = registry.emplace(name, Frontend());
    p.first->second.ready = ready;
}


auto FrontendFactory::exists(ToolName name) const -> bool
{
    return registry.find(name) != registry.cend();
}


auto FrontendFactory::available() const -> std::vector<std::string>
{
    std::vector<std::string> names;
    for (auto frontends : registry)
        names.emplace_back(frontends.first);
    return names;
}
