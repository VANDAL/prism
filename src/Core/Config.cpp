#include "Config.hpp"
#include <numeric>

namespace prism
{

auto Config::registerBackend(ToolName name,
                             BackendIfaceGenerator beGenerator,
                             BackendParser beParser,
                             BackendFinish beFinish,
                             prism::capability::EvGenCaps beRequirements) -> Config&
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    Backend be = {beGenerator, beParser, beFinish, beRequirements, {}};
    beFactory.add(name, be);
    return *this;
}


auto Config::registerFrontend(ToolName name, Frontend fe) -> Config&
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    feFactory.add(name, fe);
    return *this;
}

auto Config::parseCommandLine(int argc, char* argv[]) -> Config&
{
    Parser parser(argc, argv);

    _threads = parser.threads();
    _timed = parser.timed();

    auto execArgs = parser.executable();
    executableName = std::accumulate(std::next(execArgs.begin()), execArgs.end(), std::string{execArgs.front()},
                                     [](const std::string &a, const std::string &b) { return (a + " " + b); });

    std::vector<std::string> beArgs;
    std::tie(backendName, beArgs) = parser.backend();
    _backend = beFactory.create(backendName, beArgs);

    std::vector<std::string> feArgs;
    std::tie(frontendName, feArgs) = parser.frontend();
    _startFrontend = feFactory.create(frontendName, execArgs, feArgs, _threads, _backend.caps);

    parsed = true;

    return *this;
}

}; //end namespace prism
