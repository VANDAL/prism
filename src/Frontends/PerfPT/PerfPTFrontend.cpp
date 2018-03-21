#include "Core/PrismLog.hpp"
#include "PerfPTFrontend.hpp"
#include "FrontendShmemIPC.hpp"
#include "whereami.h"
#include <fstream>

auto perfPTCapabilities() -> prism::capabilities 
{
    using namespace prism;
    using namespace prism::capability;

    auto caps = initCaps();

    caps[MEMORY]         = availability::enabled;
    caps[MEMORY_LDST]    = availability::enabled;
    caps[MEMORY_SIZE]    = availability::enabled;
    caps[MEMORY_ADDRESS] = availability::nil;

    caps[COMPUTE]              = availability::enabled;
    caps[COMPUTE_INT_OR_FLOAT] = availability::enabled;
    caps[COMPUTE_ARITY]        = availability::nil;
    caps[COMPUTE_OP]           = availability::nil;
    caps[COMPUTE_SIZE]         = availability::nil;

    caps[CONTROL_FLOW] = availability::nil;

    caps[SYNC]      = availability::enabled;
    caps[SYNC_TYPE] = availability::enabled;
    caps[SYNC_ARGS] = availability::nil;

    caps[CONTEXT_INSTRUCTION] = availability::enabled;
    caps[CONTEXT_BASIC_BLOCK] = availability::nil;
    caps[CONTEXT_FUNCTION]    = availability::nil;
    caps[CONTEXT_THREAD]      = availability::enabled;

    return caps;
};

#ifndef PERF_ENABLE
auto startPerfPT(Args execArgs, Args feArgs, unsigned threads, prism::capabilities reqs)
    -> FrontendIfaceGenerator
{
    (void)execArgs;
    (void)feArgs;
    (void)threads;
    (void)reqs;
    PrismLog::fatal("Perf frontend not available");
}
#else

#define DIR_TEMPLATE "/prism-XXXXXX"

using PrismLog::fatal;
using PrismLog::error;
using PrismLog::warn;

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
                   2 + /* perf sigil2 options */
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

    std::string exec = std::string(path) + "/perf/bin/perf";

    /* execvp() expects a const char* const* */
    auto opts = tokenizeOpts(userExec, args, ipcDir);

    return std::make_pair(exec, opts);
}


auto configureIpcDir() -> std::string
{
    /* check IPC path */
    std::string shm_path = getenv("PRISM_SHM_DIR") != nullptr ?
                           getenv("PRISM_SHM_DIR") : "/dev/shm";

    struct stat info;
    if (stat(shm_path.c_str(), &info) != 0)
        fatal(std::string(shm_path) + " not found\n" +
              "\tset environment var 'PRISM_SHM_DIR' to a tmpfs mount");

    std::string shm_template = shm_path + DIR_TEMPLATE;
    if (mkdtemp(&shm_template[0]) == nullptr)
        fatal(std::string("creating shm dir failed -- ") + strerror(errno));

    return shm_template;
}
}; //end namespace


//-----------------------------------------------------------------------------
/** Interface to Prism core **/

auto startPerfPT(Args execArgs, Args feArgs, unsigned threads, prism::capabilities reqs)
    -> FrontendIfaceGenerator
{
    //TODO add command line switches for perf to handle capabilities
    if (threads != 1)
        fatal("Perf frontend attempted with other than 1 thread");
    auto ipcDir = configureIpcDir();
    Cleanup::setCleanupDir(ipcDir);

    auto pid = fork();
    if (pid >= 0)
    {
        if (pid == 0)
        {
            auto args = configurePerf(execArgs, feArgs, ipcDir);
            int res = execvp(args.first.c_str(), args.second);
            if (res == -1)
                fatal(std::string("starting perf failed -- ") + strerror(errno));
        }
    }
    else
        fatal(std::string("perf fork failed -- ") + strerror(errno));

    return [=]{ return std::make_unique<ShmemFrontend<PrismPerfSharedData>>(ipcDir); };
}

#endif // PERF_ENABLE
