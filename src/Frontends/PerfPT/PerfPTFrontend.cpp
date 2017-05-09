#include "Core/SigiLog.hpp"
#include "AvailableFrontends.hpp"
#include "FrontendShmemIPC.hpp"
#include "whereami.h"
#include <fstream>

#ifndef PERF_ENABLE
auto startPerfPT(FrontendStarterArgs args) -> FrontendIfaceGenerator
{
    (void)args;
    SigiLog::fatal("Perf frontend not available");
}
#else

#define DIR_TEMPLATE "/sgl2-XXXXXX"

using SigiLog::fatal;
using SigiLog::error;
using SigiLog::warn;

//-----------------------------------------------------------------------------
/** Launching Perf **/

namespace
{
using ExecArgs = char *const *;
using Exec = std::pair<std::string, ExecArgs>;


auto tokenizeOpts(const std::vector<std::string>& userExec,
                  const std::vector<std::string>& args,
                  const std::string& ipcDir) -> ExecArgs
{
    assert(!userExec.empty() && !ipcDir.empty());

    if (args.size() > 0)
        fatal("unexpected perf frontend options");

    if (userExec.size() != 1)
    {
        warn("perf frontend takes one option: perf.data file");
        fatal("collect perf.data with: perf record -e intel_pt//u <executable>");
    }

    std::string perfData(userExec.front());
    {
        std::ifstream f(userExec.front());
        if (!f.good())
            fatal("could not read file: " + userExec.front());
    }

    /* format perf options */
    /*  perf script -i <INPUT_FILE> --sigil2 <IPC DIR> -f -F comm,pid,tid,dso,ip,sym,insn,time --itrace=i1ibcrx */
    int optsSize = 1 + /* program name */
                   1 + /* 'script' */
                   2 + /* input perf data file */
                   4 + /* perf sample options */
                   1 /* null char */;

    char **opts = static_cast<char **>(malloc(optsSize * sizeof(char *)));

    int i = 0;
    opts[i++] = strdup("perf");
    opts[i++] = strdup("script");
    opts[i++] = strdup("-i");
    opts[i++] = strdup(perfData.c_str());
    opts[i++] = strdup("--sigil2");
    opts[i++] = strdup(ipcDir.c_str());
    opts[i++] = strdup("-f");
    opts[i++] = strdup("-F");
    opts[i++] = strdup("comm,pid,tid,dso,ip,sym,insn,time");
    opts[i++] = strdup("--itrace=i1ibcrx");
    opts[i] = nullptr;

    return opts;
}


auto configurePerf(const std::vector<std::string>& userExec,
                   const std::vector<std::string>& args,
                   const std::string& ipcDir) -> Exec
{
    int len, dirname_len;
    len = wai_getExecutablePath(NULL, 0, &dirname_len);
    char path[len + 1];

    if (len > 0)
    {
        wai_getExecutablePath(path, len, &dirname_len);
        path[dirname_len] = '\0';
    }
    else
    {
        fatal("couldn't find executable path");
    }

    setenv("LD_LIBRARY_PATH", std::string(path).append("/perf/lib").c_str(), true);

    std::string exec = std::string(path).append("/perf/bin/perf");

    /* execvp() expects a const char* const* */
    auto opts = tokenizeOpts(userExec, args, ipcDir);

    return std::make_pair(exec, opts);
}


auto configureIpcDir() -> std::string
{
    /* check IPC path */
    std::string shm_path = getenv("SIGIL2_SHM_DIR") != nullptr ?
                           getenv("SIGIL2_SHM_DIR") : "/dev/shm";

    struct stat info;
    if (stat(shm_path.c_str(), &info) != 0)
        fatal(std::string(shm_path) + " not found\n" +
              "\tset environment var 'SIGIL2_SHM_DIR' to a tmpfs mount");

    std::string shm_template = shm_path + DIR_TEMPLATE;
    if (mkdtemp(&shm_template[0]) == nullptr)
        fatal(std::string("creating shm dir failed -- ") + strerror(errno));

    return shm_template;
}
}; //end namespace


//-----------------------------------------------------------------------------
/** Interface to Sigil2 core **/

auto startPerfPT(FrontendStarterArgs args) -> FrontendIfaceGenerator
{
    const auto& userExecArgs = std::get<0>(args);
    const auto& perfArgs = std::get<1>(args);
    const auto& numThreads = std::get<2>(args);

    if (numThreads != 1)
        fatal("Perf frontend attempted with other than 1 thread");
    auto ipcDir = configureIpcDir();
    Cleanup::setCleanupDir(ipcDir);

    auto pid = fork();
    if (pid >= 0)
    {
        if (pid == 0)
        {
            auto args = configurePerf(userExecArgs, perfArgs, ipcDir);
            int res = execvp(args.first.c_str(), args.second);
            if (res == -1)
                fatal(std::string("starting perf failed -- ") + strerror(errno));
        }
    }
    else
        fatal(std::string("perf fork failed -- ") + strerror(errno));

    return [=]{ return std::make_unique<ShmemFrontend<Sigil2PerfSharedData>>(ipcDir); };
}

#endif // PERF_ENABLE
