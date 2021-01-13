#include "Utils/PrismLog.hpp"
#include "GengrindFrontend.hpp"
#include "FrontendShmemIPC.hpp"
#include "elfio/elfio.hpp"
#include "whereami.h"
#include "glob.h"

auto gengrindCapabilities() -> prism::capability::EvGenCaps
{
    using namespace prism;
    using namespace prism::capability;

    auto caps = initEvGenCaps();

    caps[PRISMCAP_MEMORY]              = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_MEMORY_LDST_TYPE]    = availability::PRISMCAP_ALWAYS;
    caps[PRISMCAP_MEMORY_ACCESS_BYTES] = availability::PRISMCAP_ALWAYS;
    caps[PRISMCAP_MEMORY_ADDRESS]      = availability::PRISMCAP_ALWAYS;

    caps[PRISMCAP_COMPUTE]              = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_COMPUTE_INT_OR_FLT]   = availability::PRISMCAP_ALWAYS;
    caps[PRISMCAP_COMPUTE_ARITY]        = availability::PRISMCAP_UNAVAILABLE;
    caps[PRISMCAP_COMPUTE_OP_TYPE]      = availability::PRISMCAP_UNAVAILABLE;
    caps[PRISMCAP_COMPUTE_WIDTH_BYTES]  = availability::PRISMCAP_UNAVAILABLE;

    caps[PRISMCAP_CONTROL_FLOW] = availability::PRISMCAP_UNAVAILABLE;

    caps[PRISMCAP_SYNC]      = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_SYNC_TYPE] = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_SYNC_ARGS] = availability::PRISMCAP_ENABLED;

    caps[PRISMCAP_CONTEXT_INSTRUCTION] = availability::PRISMCAP_ENABLED;
    caps[PRISMCAP_CONTEXT_BASIC_BLOCK] = availability::PRISMCAP_UNAVAILABLE;
    caps[PRISMCAP_CONTEXT_FUNCTION]    = availability::PRISMCAP_DISABLED;
    caps[PRISMCAP_CONTEXT_THREAD]      = availability::PRISMCAP_ENABLED;

    return caps;
};

#define DIR_TEMPLATE "/prism-XXXXXX"

using PrismLog::fatal;
using PrismLog::warn;

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


auto configureWrapperEnv(const std::string &prism_path) -> void
{
    /* check if function capture is available
     * (for multithreaded lib intercepts) */
    std::string prismvgwrapper(prism_path + "/libprismvgwrapper.so");
    std::ifstream sofile(prismvgwrapper);

    if (sofile.good() == true)
    {
        const char *get_preload = getenv("LD_PRELOAD");
        std::string set_preload;

        if (get_preload == nullptr)
            set_preload = prismvgwrapper;
        else
            set_preload = std::string(get_preload) + ":" + prismvgwrapper;

        setenv("LD_PRELOAD", set_preload.c_str(), true);
    }
    else
    {
        /* If the wrapper library is in LD_PRELOAD,
         * but not in the prism directory,
         * then this warning can be ignored */
        warn("'prismvgwrapper.so' not found");
        warn("Synchronization events will not be detected without the wrapper library loaded");
    }

    sofile.close();
}


auto vgOpts(const std::vector<std::string> &userExec,
            const std::vector<std::string> &args,
            const std::string &ipcDir,
            const prism::capability::EvGenCaps &reqs) -> std::string
{
    using namespace prism::capability;
    assert(!userExec.empty() && !ipcDir.empty());

    std::string opts;
    opts += " --fair-sched=yes"; /* more reliable and reproducible
                                   thread interleaving; round robins
                                   each thread instead of letting one
                                   thread dominate execution */
    opts += " --tool=sigrind";

    opts += " --ipc-dir=" + ipcDir;

    /* MDL20170720
     * TODO(someday)
     * The Valgrind frontend only has granularity support at launch,
     * but not during execution. Would need to establish some type of
     * gdbserver connection to valgrind to poke it to change granularity
     * on-the-fly, or have discrete points to change granularities
     * (like at UDST or specific function) */
    reqs[PRISMCAP_MEMORY] == availability::PRISMCAP_ENABLED ?
        opts += " --gen-mem=yes" :
        opts += " --gen-mem=no";
    reqs[PRISMCAP_COMPUTE] == availability::PRISMCAP_ENABLED ?
        opts += " --gen-comp=yes" :
        opts += " --gen-comp=no";
    reqs[PRISMCAP_SYNC] == availability::PRISMCAP_ENABLED ?
        opts += " --gen-sync=yes" :
        opts += " --gen-sync=no";
    reqs[PRISMCAP_CONTEXT_INSTRUCTION] == availability::PRISMCAP_ENABLED ?
        opts += " --gen-instr=yes" :
        opts += " --gen-instr=no";
    reqs[PRISMCAP_CONTEXT_BASIC_BLOCK] == availability::PRISMCAP_ENABLED ?
        opts += " --gen-bb=yes" :
        opts += " --gen-bb=no";
    reqs[PRISMCAP_CONTEXT_FUNCTION] == availability::PRISMCAP_ENABLED ?
        opts += " --gen-fn=yes" :
        opts += " --gen-fn=no";
    opts += " --gen-cf=no";

    /* command line arguments will override capabilities */
    for (auto &arg : args)
        opts += " " + arg;

    for (auto &arg : userExec)
        opts += " " + arg;

    return opts;
}


auto configureValgrind(const std::vector<std::string> &userExec,
                       const std::vector<std::string> &args,
                       const std::string &ipcDir,
                       const prism::capability::EvGenCaps &reqs) -> std::string
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

    std::string exec = std::string(path).append("/vg/bin/valgrind");

    /* execvp() expects a const char* const* */
    auto opts = vgOpts(userExec, args, ipcDir, reqs);

    return exec + " " + opts;
}


auto configureIpcDir() -> std::string
{
    /* check IPC path */
    std::string shm_path = getenv("PRISM_SHM_DIR") != nullptr ?  getenv("PRISM_SHM_DIR") :
                           getenv("XDG_RUNTIME_DIR") != nullptr ? getenv("XDG_RUNTIME_DIR") :
                           "/dev/shm";
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


////////////////////////////////////////////////////////////
// Interface to Prism core
////////////////////////////////////////////////////////////
auto startGengrind(Args execArgs,
                   Args feArgs,
                   unsigned threads,
                   prism::capability::EvGenCaps reqs) -> FrontendIfaceGenerator
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
            const char* bash = "bash";
            std::string valgrindArgs = configureValgrind(execArgs, feArgs, ipcDir, reqs);

            /* format dynamorio options */
            int bashOptsSize = 4;
            char **bashOpts = static_cast<char **>(malloc(bashOptsSize * sizeof(char *)));
            int i = 0;
            bashOpts[i++] = strdup("bash");
            bashOpts[i++] = strdup("-c");
            bashOpts[i++] = strdup(valgrindArgs.c_str());
            bashOpts[i++] = nullptr;

            // Bash is preferable because it does all the expected
            // command line string parsing and i/o redirection.
            // Less surprises.
            int res = execvp(bash, bashOpts);
            if (res == -1)
                fatal(std::string("starting valgrind failed -- ") + strerror(errno));
        }
    }
    else
        fatal(std::string("sigrind fork failed -- ") + strerror(errno));

    return [=]{ return std::make_unique<ShmemFrontend<PrismDBISharedData>>(ipcDir); };
}
