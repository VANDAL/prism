#include "Config.hpp"
#include <numeric>

namespace sigil2
{

auto Config::registerBackend(ToolName name, Backend be) -> Config&
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
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
    executableName = std::accumulate(execArgs.begin()+1, execArgs.end(), std::string{execArgs.front()},
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

}; //end namespace sigil2
