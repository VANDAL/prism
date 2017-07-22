#include "Backends.hpp"
#include "SigiLog.hpp"
#include <algorithm>

auto BackendFactory::create(ToolName name, Args args) const -> Backend
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name.empty() == false && exists(name))
    {
        auto backend = registry.find(name)->second;
        backend.args = args;
        return backend;
    }
    else
    {
        std::string error("Invalid backend argument: ");
        error.append(name + "\n");
        error.append("\tAvailable backends: ");

        for (auto name : available())
            error.append("\n\t").append(name);

        SigiLog::fatal(error);
    }
}


auto BackendFactory::add(ToolName name, Backend be) -> void
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    auto p = registry.emplace(name, Backend());
    p.first->second = be;
}


auto BackendFactory::exists(ToolName name) const -> bool
{
    return registry.find(name) != registry.cend();
}


auto BackendFactory::available() const -> std::vector<std::string>
{
    std::vector<std::string> names;
    for (auto backends : registry)
        names.emplace_back(backends.first);
    return names;
}
