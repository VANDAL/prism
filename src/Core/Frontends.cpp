#include "Frontends.hpp"
#include "SigiLog.hpp"
#include <algorithm>

decltype(FrontendIface::uidCount) FrontendIface::uidCount{0};

auto FrontendFactory::create(ToolName name, Args exec, Args fe, unsigned threads,
                             const sigil2::capabilities &beReqs) const -> FrontendStarterWrapper
{
    using namespace std::placeholders;

    /* default */
    if (name.empty() == true)
        name = "valgrind";

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (exists(name) == true)
    {
        auto start = registry.find(name)->second.starter;
        auto feCaps = registry.find(name)->second.caps;

        /* Resolve difference between requested capabilities (granularity)
         * from the backend, and the available capabilities in the frontend */
        auto caps = sigil2::resolveCaps(feCaps, beReqs);;
        return [=]{ return start(exec, fe, threads, caps); };
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


auto FrontendFactory::add(ToolName name, Frontend fe) -> void
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    auto p = registry.emplace(name, Frontend());
    p.first->second = fe;
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
