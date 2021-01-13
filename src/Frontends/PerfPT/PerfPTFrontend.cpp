#include "Utils/PrismLog.hpp"
#include "PerfPTFrontend.hpp"
#include "FrontendShmemIPC.hpp"
#include "whereami.h"
#include <fstream>

auto perfPTCapabilities() -> prism::capability::EvGenCaps 
{
    using namespace prism;
    using namespace prism::capability;

    auto caps = initEvGenCaps();

    caps[PRISMCAP_MEMORY]              = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_MEMORY_LDST_TYPE]    = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_MEMORY_ACCESS_BYTES] = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_MEMORY_ADDRESS]      = availability::PRISMCAP_UNAVAILABLE;

    caps[PRISMCAP_COMPUTE]             = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_COMPUTE_INT_OR_FLT]  = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_COMPUTE_ARITY]       = availability::PRISMCAP_UNAVAILABLE;
    caps[PRISMCAP_COMPUTE_OP_TYPE]     = availability::PRISMCAP_UNAVAILABLE;
    caps[PRISMCAP_COMPUTE_WIDTH_BYTES] = availability::PRISMCAP_UNAVAILABLE;

    caps[PRISMCAP_CONTROL_FLOW] = availability::PRISMCAP_UNAVAILABLE;

    caps[PRISMCAP_SYNC]      = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_SYNC_TYPE] = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_SYNC_ARGS] = availability::PRISMCAP_UNAVAILABLE;

    caps[PRISMCAP_CONTEXT_INSTRUCTION] = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_CONTEXT_BASIC_BLOCK] = availability::PRISMCAP_UNAVAILABLE;
    caps[PRISMCAP_CONTEXT_FUNCTION]    = availability::PRISMCAP_UNAVAILABLE;
    caps[PRISMCAP_CONTEXT_THREAD]      = availability::PRISMCAP_ENABLED;

    return caps;
};

#ifndef PERF_ENABLE
auto startPerfPT(Args execArgs,
                 Args feArgs,
                 unsigned threads,
                 prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator
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
