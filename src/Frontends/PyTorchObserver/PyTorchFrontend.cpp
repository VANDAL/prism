#include "Utils/PrismLog.hpp"
#include "PyTorchFrontend.hpp"
#include "FrontendShmemIPC.hpp"
#include "whereami.h"
#include "glob.h"

auto PyTorchCapabilities() -> prism::capability::EvGenCaps
{
    using namespace prism;
    using namespace prism::capability;

    auto caps = initEvGenCaps();

    caps[PRISMCAP_MEMORY]              = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_MEMORY_LDST_TYPE]    = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_MEMORY_ACCESS_BYTES] = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_MEMORY_ADDRESS]      = availability::PRISMCAP_DISABLED;

    caps[PRISMCAP_COMPUTE]             = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_COMPUTE_INT_OR_FLT]  = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_COMPUTE_ARITY]       = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_COMPUTE_OP_TYPE]     = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_COMPUTE_WIDTH_BYTES] = availability::PRISMCAP_DISABLED;

    caps[PRISMCAP_CONTROL_FLOW] = availability::PRISMCAP_DISABLED;

    caps[PRISMCAP_SYNC]      = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_SYNC_TYPE] = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_SYNC_ARGS] = availability::PRISMCAP_DISABLED;

    caps[PRISMCAP_CONTEXT_INSTRUCTION] = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_CONTEXT_BASIC_BLOCK] = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_CONTEXT_FUNCTION]    = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_CONTEXT_THREAD]      = availability::PRISMCAP_DISABLED;

    return caps;
};

#ifndef PYTORCH_ENABLE
auto startPyTorchCaffe2(
    Args execArgs,
    Args feArgs,
    unsigned threads,
    prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator
{
    (void)execArgs;
    (void)feArgs;
    (void)threads;
    (void)reqs;
    PrismLog::fatal("PyTorch frontend not available");
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


auto tokenizeOpts(
    const std::vector<std::string>& pythonArgs,
    const std::string& ipcDir,
    const prism::capability::EvGenCaps &reqs) -> ExecArgs
{
    std::vector<std::string> args;
    args.push_back("python");
    args.push_back("-u");

    // the script to execute
    args.push_back(pythonArgs.front());

    // needed for IPC
    args.push_back("--prism_ipc_dir=" + ipcDir);  

    // rest of the user supplied args
    args.insert(args.end(), pythonArgs.begin()+1, pythonArgs.end());

    /* format options */
    int optsLen = args.size() + 1;
    char **opts = static_cast<char **>(malloc(optsLen * sizeof(char *)));

    for (size_t i = 0; i < args.size(); i++) {
        opts[i] = strdup(args[i].c_str());
    }
    opts[args.size()] = nullptr;

    return opts;
}


auto configurePyTorchCaffe2(
    const std::vector<std::string> &pythonArgs,
    const std::vector<std::string> &feArgs,
    const std::string &ipcDir,
    const uint16_t num_threads,
    const prism::capability::EvGenCaps &reqs) -> Exec
{
    if (num_threads != 1) {
        fatal("PyTorch/Caffe2 frontend does not yet support multithreading the PyTorch/Caffe2 instance.");
    }

    if (feArgs.size() > 0) {
        fatal("Given args, but no args expected for PyTorch/Caffe2 frontend.");
    }

    // No args exist right now for this frontend.
    // Expects most args to be put in supplied python script for simplicity.
    auto exec = std::string{"python"};

    /* execvp() expects a const char* const* */
    auto opts = tokenizeOpts(pythonArgs, ipcDir, reqs);

    return std::make_pair(exec, opts);
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
auto startPyTorchCaffe2(
    Args execArgs,
    Args feArgs,
    unsigned threads,
    prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator
{
    auto ipcDir = configureIpcDir();
    Cleanup::setCleanupDir(ipcDir);

    // create fifos
    auto fe = std::make_unique<ShmemFrontend<PrismDBISharedData>>(ipcDir, false);

    // blocking open on rdonly fifo after frontend starts, and opens fifo as wronly
    auto ifaceGenerator = [p=std::move(fe)]() mutable {
        p->openFifos();
        return std::move(p); 
    };

    auto pid = fork();
    if (pid >= 0)
    {
        if (pid == 0)
        {
            auto args = configurePyTorchCaffe2(execArgs, feArgs, ipcDir, threads, reqs);
            int res = execvp(args.first.c_str(), args.second);
            if (res == -1)
                fatal(std::string("starting dynamorio failed -- ").append(strerror(errno)));
        }
    }
    else
        fatal(std::string("sigrind fork failed -- ") + strerror(errno));

    // hack: non-copyable lambda -> std::function
    return [p=std::make_shared<decltype(ifaceGenerator)>(std::move(ifaceGenerator))] {return (*p)();};
}

#endif
