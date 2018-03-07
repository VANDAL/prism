#include "Core/SigiLog.hpp"
#include "SigrindFrontend.hpp"
#include "FrontendShmemIPC.hpp"
#include "elfio/elfio.hpp"
#include "whereami.h"
#include "glob.h"

auto sigrindCapabilities() -> sigil2::capabilities 
{
    using namespace sigil2;
    using namespace sigil2::capability;

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
    caps[CONTEXT_BASIC_BLOCK] = availability::disabled;
    caps[CONTEXT_FUNCTION]    = availability::disabled;
    caps[CONTEXT_THREAD]      = availability::enabled;

    return caps;
};

#define DIR_TEMPLATE "/sgl2-XXXXXX"

using SigiLog::fatal;
using SigiLog::warn;

////////////////////////////////////////////////////////////
// Launching Valgrind
////////////////////////////////////////////////////////////
namespace
{
using ExecArgs = char *const *;
using Exec = std::pair<std::string, ExecArgs>;

auto gccWarn(const std::vector<std::string> &userExec) -> void
{
    assert(userExec.empty() == false);

    /* Naively assume the first option is the user binary.
     * ML: KS says that OpenMP is only guaranteed to work for
     * GCC 4.9.2. Pthreads should work for most recent GCC
     * releases. Cannot check if file exists because it is
     * not guaranteed that this string is actually the binary */
    ELFIO::elfio reader;
    bool is_gcc_compatible = false;
    std::string gcc_version_needed("4.9.2");
    std::string gcc_version_found;

    if (reader.load(userExec[0]) != 0)
    {
        ELFIO::Elf_Half sec_num = reader.sections.size();

        for (int i = 0; i < sec_num; ++i)
        {
            ELFIO::section *psec = reader.sections[i];

            if (psec->get_name().compare(".comment") == 0)
            {
                const char *p = reader.sections[i]->get_data();

                if (p != nullptr)
                {
                    /* Check for "GCC: (GNU) X.X.X" */
                    std::string comment(p);
                    size_t pos = comment.find_last_of(')');

                    if (pos + 2 < comment.size())
                    {
                        gcc_version_found = comment.substr(pos + 2);

                        if (gcc_version_found.compare(gcc_version_needed) == 0)
                        {
                            is_gcc_compatible = true;
                        }
                    }
                }

                break;
            }
        }
    }

    if (is_gcc_compatible == false)
    {
        warn("\'" + userExec[0] + "\'" + ":");
        warn("GCC version " + gcc_version_needed + " not detected");

        if (gcc_version_found.empty() == false)
            warn("GCC version " + gcc_version_found + " found");
        else
            warn("GCC version could not be detected");

        warn("OpenMP synchronization events may not be captured");
        warn("Pthread synchronization events are probably fine");
    }
}


auto configureWrapperEnv(const std::string &sigil2_path) -> void
{
    /* check if function capture is available
     * (for multithreaded lib intercepts) */
    std::string sglwrapper(sigil2_path + "/libsglwrapper.so");
    std::ifstream sofile(sglwrapper);

    if (sofile.good() == true)
    {
        const char *get_preload = getenv("LD_PRELOAD");
        std::string set_preload;

        if (get_preload == nullptr)
            set_preload = sglwrapper;
        else
            set_preload = std::string(get_preload) + ":" + sglwrapper;

        setenv("LD_PRELOAD", set_preload.c_str(), true);
    }
    else
    {
        /* If the wrapper library is in LD_PRELOAD,
         * but not in the sigil2 directory,
         * then this warning can be ignored */
        warn("'sglwrapper.so' not found");
        warn("Synchronization events will not be detected without the wrapper library loaded");
    }

    sofile.close();
}


auto tokenizeOpts(const std::vector<std::string> &userExec,
                  const std::vector<std::string> &args,
                  const std::string &ipcDir,
                  const sigil2::capabilities &reqs) -> ExecArgs
{
    using namespace sigil2::capability;
    assert(!userExec.empty() && !ipcDir.empty());

    /* format valgrind options */
    int vg_opts_size = 1/*program name*/ +
                       2/*vg opts*/ +
                       1/*ipc_dir*/ +
                       8/*sigrind opts default*/ +
                       args.size()/*sigrind opts defined*/ +
                       userExec.size()/*user program options*/ +
                       1/*null*/;
    char **vg_opts = static_cast<char **>(malloc(vg_opts_size * sizeof(char *)));

    int i = 0;
    /*program name*/
    vg_opts[i++] = strdup("valgrind");

    /*vg opts*/
    vg_opts[i++] = strdup("--fair-sched=yes"); /* more reliable and reproducible
                                                  thread interleaving; round robins
                                                  each thread instead of letting one
                                                  thread dominate execution */
    vg_opts[i++] = strdup("--tool=sigrind");

    vg_opts[i++] = strdup(("--ipc-dir=" + ipcDir).c_str());

    /* MDL20170720
     * TODO the Valgrind frontend only has macro granularity support */
    reqs[MEMORY] == availability::enabled ?
        vg_opts[i++] = strdup("--gen-mem=yes") :
        vg_opts[i++] = strdup("--gen-mem=no");
    reqs[COMPUTE] == availability::enabled ?
        vg_opts[i++] = strdup("--gen-comp=yes") :
        vg_opts[i++] = strdup("--gen-comp=no");
    reqs[SYNC] == availability::enabled ?
        vg_opts[i++] = strdup("--gen-sync=yes") :
        vg_opts[i++] = strdup("--gen-sync=no");
    reqs[CONTEXT_INSTRUCTION] == availability::enabled ?
        vg_opts[i++] = strdup("--gen-instr=yes") :
        vg_opts[i++] = strdup("--gen-instr=no");
    reqs[CONTEXT_BASIC_BLOCK] == availability::enabled ?
        vg_opts[i++] = strdup("--gen-bb=yes") :
        vg_opts[i++] = strdup("--gen-bb=no");
    reqs[CONTEXT_FUNCTION] == availability::enabled ?
        vg_opts[i++] = strdup("--gen-fn=yes") :
        vg_opts[i++] = strdup("--gen-fn=no");
    vg_opts[i++] = strdup("--gen-cf=no");

    /* command line arguments will override capabilities */
    for (auto &arg : args)
        vg_opts[i++] = strdup(arg.c_str());

    for (auto &arg : userExec)
        vg_opts[i++] = strdup(arg.c_str());

    vg_opts[i] = nullptr;

    return vg_opts;
}


auto configureValgrind(const std::vector<std::string> &userExec,
                       const std::vector<std::string> &args,
                       const std::string &ipcDir,
                       const sigil2::capabilities &reqs) -> Exec
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

    /* set up valgrind function wrapping to capture synchronization */
    configureWrapperEnv(path);

    /* XXX HACK if the user decides to move the install folder, valgrind will
     * get confused and require that VALGRIND_LIB be set.  Set this variable for
     * the user to avoid confusion */
    setenv("VALGRIND_LIB", std::string(path).append("/vg/lib/valgrind").c_str(), true);

    std::string vg_exec = std::string(path).append("/vg/bin/valgrind");

    /* execvp() expects a const char* const* */
    auto vg_opts = tokenizeOpts(userExec, args, ipcDir, reqs);

    return std::make_pair(vg_exec, vg_opts);
}


auto configureIpcDir() -> std::string
{
    /* check IPC path */
    std::string shm_path = getenv("SIGIL2_SHM_DIR") != nullptr ?  getenv("SIGIL2_SHM_DIR") :
                           getenv("XDG_RUNTIME_DIR") != nullptr ? getenv("XDG_RUNTIME_DIR") :
                           "/dev/shm";
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


////////////////////////////////////////////////////////////
// Interface to Sigil2 core
////////////////////////////////////////////////////////////
auto startSigrind(Args execArgs, Args feArgs, unsigned threads, sigil2::capabilities reqs)
    -> FrontendIfaceGenerator
{
    if (threads != 1)
        fatal("Valgrind frontend attempted with other than 1 thread");
    gccWarn(execArgs);
    auto ipcDir = configureIpcDir();
    Cleanup::setCleanupDir(ipcDir);

    auto pid = fork();
    if (pid >= 0)
    {
        if (pid == 0)
        {
            auto valgrindArgs = configureValgrind(execArgs, feArgs, ipcDir, reqs);
            int res = execvp(valgrindArgs.first.c_str(), valgrindArgs.second);
            if (res == -1)
                fatal(std::string("starting valgrind failed -- ") + strerror(errno));
        }
    }
    else
        fatal(std::string("sigrind fork failed -- ") + strerror(errno));

    return [=]{ return std::make_unique<ShmemFrontend<Sigil2DBISharedData>>(ipcDir); };
}
