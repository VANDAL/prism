#include "Utils/PrismLog.hpp"
#include "DrSigilFrontend.hpp"
#include "FrontendShmemIPC.hpp"
#include "whereami.h"
#include "glob.h"

auto drSigilCapabilities() -> prism::capabilities
{
    using namespace prism;
    using namespace prism::capability;

    auto caps = initCaps();

    caps[MEMORY]         = availability::enabled;
    caps[MEMORY_LDST]    = availability::enabled;
    caps[MEMORY_SIZE]    = availability::enabled;
    caps[MEMORY_ADDRESS] = availability::enabled;

    caps[COMPUTE]              = availability::enabled;
    caps[COMPUTE_INT_OR_FLOAT] = availability::enabled;
    caps[COMPUTE_ARITY]        = availability::nil;
    caps[COMPUTE_OP]           = availability::nil;
    caps[COMPUTE_SIZE]         = availability::nil;

    caps[CONTROL_FLOW] = availability::nil;

    caps[SYNC]      = availability::enabled;
    caps[SYNC_TYPE] = availability::enabled;
    caps[SYNC_ARGS] = availability::enabled;

    caps[CONTEXT_INSTRUCTION] = availability::enabled;
    caps[CONTEXT_BASIC_BLOCK] = availability::nil;
    caps[CONTEXT_FUNCTION]    = availability::nil;
    caps[CONTEXT_THREAD]      = availability::enabled;

    return caps;
};

#ifndef DYNAMORIO_ENABLE
auto startDrSigil(Args execArgs, Args feArgs, unsigned threads, prism::capabilities reqs)
    -> FrontendIfaceGenerator
{
    (void)execArgs;
    (void)feArgs;
    (void)threads;
    (void)reqs;
    PrismLog::fatal("DynamoRIO frontend not available");
}
#else

#define DIR_TEMPLATE "/prism-XXXXXX"

using PrismLog::fatal;

////////////////////////////////////////////////////////////
// Launching DynamoRIO
////////////////////////////////////////////////////////////
namespace
{
using ExecArgs = char *const *;
using Exec = std::pair<std::string, ExecArgs>;


auto getDynamoRIOArgs(const std::string &sigilBinDir,
                      const std::string &ipcDir,
                      uint16_t threads,
                      const prism::capabilities &reqs) -> Args
{
    using namespace prism;
    using namespace prism::capability;

    Args drArgs;

    drArgs.push_back("drrun");
    //drArgs.push_back("debug");
    drArgs.push_back("-root");
    drArgs.push_back(sigilBinDir + "/dr");
    drArgs.push_back("-max_bb_instrs");
    drArgs.push_back("128");
    drArgs.push_back("-c");

    /* detect 32/64 bit and release/debug build */
    glob_t glob_result;
    glob((sigilBinDir + "/dr/tools/lib*/*").c_str(),
         GLOB_MARK|GLOB_TILDE|GLOB_ONLYDIR,
         NULL, &glob_result);
    if (glob_result.gl_pathc != 1)
        fatal("Error detecting \'libdrsigil.so\' path");
    std::string drLib = std::string(glob_result.gl_pathv[0]);
    globfree(&glob_result);

    drArgs.push_back(drLib + "libdrsigil.so");
    drArgs.push_back("--num-frontend-threads=" + std::to_string(threads));
    drArgs.push_back("--ipc-dir=" + ipcDir);

    if (reqs[MEMORY] == availability::enabled)
        drArgs.push_back("--enable-mem");
    if (reqs[MEMORY_LDST] == availability::enabled)
        drArgs.push_back("--enable-mem-type");
    if (reqs[MEMORY_SIZE] == availability::enabled)
        drArgs.push_back("--enable-mem-size");
    if (reqs[MEMORY_ADDRESS] == availability::enabled)
        drArgs.push_back("--enable-mem-addr");

    if (reqs[COMPUTE] == availability::enabled)
        drArgs.push_back("--enable-comp");
    if (reqs[COMPUTE_INT_OR_FLOAT] == availability::enabled)
        drArgs.push_back("--enable-comp-type");

    if (reqs[SYNC] == availability::enabled)
        drArgs.push_back("--enable-sync");
    if (reqs[SYNC_TYPE] == availability::enabled)
        drArgs.push_back("--enable-sync-type");
    if (reqs[SYNC_ARGS] == availability::enabled)
        drArgs.push_back("--enable-sync-data");

    if (reqs[CONTEXT_INSTRUCTION] == availability::enabled)
        drArgs.push_back("--enable-context-instr");

    return drArgs;
}


auto tokenizeOpts(const std::vector<std::string> &userExec,
                  const std::vector<std::string> &args,
                  const std::string &sigilBinDir,
                  const std::string &ipcDir,
                  const uint16_t threads,
                  const prism::capabilities &reqs) -> ExecArgs
{
    assert(!userExec.empty() && !ipcDir.empty());

    auto drArgs = getDynamoRIOArgs(sigilBinDir, ipcDir, threads, reqs);

    /* format dynamorio options */
    int drOptsSize = drArgs.size()   + // dynamorio options
                     args.size()     + // extra frontend options
                     1               + // '--' to separate user program to DR
                     userExec.size() + // user program options
                     1;                // null
    char **drOpts = static_cast<char **>(malloc(drOptsSize * sizeof(char *)));

    int i = 0;
    for (auto &arg : drArgs)
        drOpts[i++] = strdup(arg.c_str());

    /* assume extra frontend options are directly passed to DR */
    for (auto &arg : args)
        drOpts[i++] = strdup(arg.c_str());

    drOpts[i++] = strdup("--");

    for (auto &arg : userExec)
        drOpts[i++] = strdup(arg.c_str());

    drOpts[i] = nullptr;

    return drOpts;
}


auto configureDynamoRIO(const std::vector<std::string> &user_exec,
                        const std::vector<std::string> &args,
                        const std::string &ipc_dir,
                        const uint16_t num_threads,
                        const prism::capabilities &reqs) -> Exec
{
    int dirname_len;
    int len = wai_getExecutablePath(NULL, 0, &dirname_len);
    if (len <= 0)
        fatal("couldn't find executable path");

    char path[len + 1];
    wai_getExecutablePath(path, len, &dirname_len);
    path[dirname_len] = '\0';

    /* detect 32/64 bit */
    glob_t glob_result;
    glob(std::string(path).append("/dr/bin*").c_str(),
         GLOB_MARK|GLOB_TILDE|GLOB_ONLYDIR,
         NULL, &glob_result);

    if (glob_result.gl_pathc != 1)
        fatal("Error detecting \'drrun\' path");

    std::string dr_exec = std::string(glob_result.gl_pathv[0]) + ("drrun");
    globfree(&glob_result);

    /* execvp() expects a const char* const* */
    auto dr_opts = tokenizeOpts(user_exec, args, path, ipc_dir, num_threads, reqs);

    return std::make_pair(dr_exec, dr_opts);
}


auto configureIpcDir() -> std::string
{
    /* check IPC path */
    std::string shm_path = getenv("PRISM_SHM_DIR") != nullptr ?  getenv("PRISM_SHM_DIR") :
                           getenv("XDG_RUNTIME_DIR") != nullptr ? getenv("XDG_RUNTIME_DIR") :
                           "/dev/shm";
    struct stat info;
    if (stat(shm_path.c_str(), &info) != 0)
        fatal(std::string{shm_path} + " not found\n" +
              "\tset environment var 'PRISM_SHM_DIR' to a tmpfs mount");

    std::string shm_template = shm_path + DIR_TEMPLATE;
    if (mkdtemp(&shm_template[0]) == nullptr)
        fatal(std::string("creating shm dir failed -- ") + strerror(errno));

    return shm_template;
}
}; //end namespace


////////////////////////////////////////////////////////////
// Interface to Prism core
////////////////////////////////////////////////////////////
auto startDrSigil(Args execArgs, Args feArgs, unsigned threads, prism::capabilities reqs)
    -> FrontendIfaceGenerator
{
    auto ipcDir = configureIpcDir();
    Cleanup::setCleanupDir(ipcDir);

    auto pid = fork();
    if (pid >= 0)
    {
        if (pid == 0)
        {
            auto drArgs = configureDynamoRIO(execArgs, feArgs, ipcDir, threads, reqs);
            int res = execvp(drArgs.first.c_str(), drArgs.second);
            if (res == -1)
                fatal(std::string("starting dynamorio failed -- ").append(strerror(errno)));
        }
    }
    else
        fatal(std::string("sigrind fork failed -- ") + strerror(errno));

    return [=]{ return std::make_unique<ShmemFrontend<PrismDBISharedData>>(ipcDir); };
}

#endif
