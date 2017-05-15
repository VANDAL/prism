#include "Core/SigiLog.hpp"
#include "AvailableFrontends.hpp"
#include "FrontendShmemIPC.hpp"
#include "whereami.h"
#include "glob.h"

#ifndef DYNAMORIO_ENABLE
auto startDrSigil(FrontendStarterArgs args) -> FrontendIfaceGenerator
{
    (void)args;
    SigiLog::fatal("DynamoRIO frontend not available");
}
#else

#define DIR_TEMPLATE "/sgl2-XXXXXX"

using SigiLog::fatal;

////////////////////////////////////////////////////////////
// Launching DynamoRIO
////////////////////////////////////////////////////////////
namespace
{
using ExecArgs = char *const *;
using Exec = std::pair<std::string, ExecArgs>;

auto tokenizeOpts(const std::vector<std::string> &user_exec,
                  const std::vector<std::string> &args,
                  const std::string &sigil_bin_dir,
                  const std::string &ipc_dir,
                  const uint16_t num_threads) -> ExecArgs
{
    assert(!user_exec.empty() && !ipc_dir.empty());

    /* format dynamorio options */
    //                 program name + dynamorio options + user program options + null
    int dr_opts_size = 1            + 10 + args.size()     + user_exec.size()        + 1;
    char **dr_opts = static_cast<char **>(malloc(dr_opts_size * sizeof(char *)));

    int i = 0;
    dr_opts[i++] = strdup("drrun");
    //dr_opts[i++] = strdup("-debug");
    dr_opts[i++] = strdup("-root");
    dr_opts[i++] = strdup((sigil_bin_dir + "/dr").c_str());
    dr_opts[i++] = strdup("-max_bb_instrs");
    dr_opts[i++] = strdup("128");
    dr_opts[i++] = strdup("-c");

    /* detect 32/64 bit and release/debug build */
    glob_t glob_result;
    glob(std::string(sigil_bin_dir).append("/dr/tools/lib*/*").c_str(),
         GLOB_MARK|GLOB_TILDE|GLOB_ONLYDIR,
         NULL, &glob_result);

    if (glob_result.gl_pathc != 1)
        fatal("Error detecting \'libdrsigil.so\' path");

    std::string dr_lib = std::string(glob_result.gl_pathv[0]);

    globfree(&glob_result);

    dr_opts[i++] = strdup((dr_lib + "libdrsigil.so").c_str());
    dr_opts[i++] = strdup(("--num-frontend-threads=" + std::to_string(num_threads)).c_str());
    dr_opts[i++] = strdup(("--ipc-dir=" + ipc_dir).c_str());

    for (auto &arg : args)
        dr_opts[i++] = strdup(arg.c_str());

    dr_opts[i++] = strdup("--");

    for (auto &arg : user_exec)
        dr_opts[i++] = strdup(arg.c_str());

    dr_opts[i] = nullptr;

    return dr_opts;
}


auto configureDynamoRIO(const std::vector<std::string> &user_exec,
                        const std::vector<std::string> &args,
                        const std::string &ipc_dir,
                        const uint16_t num_threads) -> Exec
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
    auto dr_opts = tokenizeOpts(user_exec, args, path, ipc_dir, num_threads);

    return std::make_pair(dr_exec, dr_opts);
}


auto configureIpcDir() -> std::string
{
    /* check IPC path */
    std::string shm_path = getenv("SIGIL2_SHM_DIR") != nullptr ?
                           getenv("SIGIL2_SHM_DIR") : "/dev/shm";
    struct stat info;
    if (stat(shm_path.c_str(), &info) != 0)
        fatal(std::string{shm_path} + " not found\n" +
              "\tset environment var 'SIGIL2_SHM_DIR' to a tmpfs mount");

    std::string shm_template = shm_path + DIR_TEMPLATE;
    if (mkdtemp(&shm_template[0]) == nullptr)
        fatal(std::string("creating shm dir failed -- ") + strerror(errno));

    return shm_template;
}
}; //end namespace


////////////////////////////////////////////////////////////
// Interface to Sigil2 core
////////////////////////////////////////////////////////////
auto startDrSigil(FrontendStarterArgs args) -> FrontendIfaceGenerator
{
    auto ipcDir = configureIpcDir();
    Cleanup::setCleanupDir(ipcDir);

    auto pid = fork();
    if (pid >= 0)
    {
        if (pid == 0)
        {
            const auto& userExecArgs = std::get<0>(args);
            const auto& drSigilArgs  = std::get<1>(args);
            const auto& numThreads   = std::get<2>(args);
            auto drArgs = configureDynamoRIO(userExecArgs, drSigilArgs, ipcDir, numThreads);
            int res = execvp(drArgs.first.c_str(), drArgs.second);
            if (res == -1)
                fatal(std::string("starting dynamorio failed -- ").append(strerror(errno)));
        }
    }
    else
        fatal(std::string("sigrind fork failed -- ") + strerror(errno));

    return [=]{ return std::make_unique<ShmemFrontend<Sigil2DBISharedData>>(ipcDir); };
}

#endif // DYNAMORIO_ENABLE
