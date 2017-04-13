#include "Frontends.hpp"
#include "SigiLog.hpp"
#include <algorithm>

decltype(FrontendIface::uidCount) FrontendIface::uidCount{0};

auto FrontendFactory::create(ToolName name, FrontendStarterArgs args) const -> FrontendStarterWrapper
{
    /* default */
    if (name.empty() == true)
        name = "valgrind";

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (exists(name) == true)
    {
        auto start = registry.find(name)->second;
        return [=]{ return start(args); };
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
    registry.emplace(name, start);
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
