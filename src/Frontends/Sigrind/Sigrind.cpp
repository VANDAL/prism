#include "Sigrind.hpp"
#include "Sigil2/SigiLog.hpp"
#include "whereami.h"
#include "elfio/elfio.hpp"

#include <csignal>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define DIR_TEMPLATE "/sgl2-vg-XXXXXX"

namespace sgl
{

using SigiLog::warn;
using SigiLog::fatal;

namespace
{
/* signal handler needs this to know which files to clean up */
std::string shadow_ipcDir;
std::string shadow_shmemName;
std::string shadow_emptyFifoName;
std::string shadow_fullFifoName;

/* One interface to Valgrind */
std::atomic<bool> _sigrindReady{false};
std::shared_ptr<Sigrind> sigrindIface;
Sem complete{0};
};

////////////////////////////////////////////////////////////
// Sigil2 - Valgrind IPC
////////////////////////////////////////////////////////////
Sigrind::Sigrind(int threads, const std::string &ipcDir)
    : ipcDir(ipcDir)
    , shmemName(ipcDir + "/" + SIGRIND_SHMEM_NAME)
    , emptyFifoName(ipcDir + "/" + SIGRIND_EMPTYFIFO_NAME)
    , fullFifoName (ipcDir + "/" + SIGRIND_FULLFIFO_NAME)
    , threads(threads)
    , idx(0)
{
    assert(threads == 1);

    shadow_ipcDir        = ipcDir;
    shadow_shmemName     = shmemName;
    shadow_fullFifoName  = fullFifoName;
    shadow_emptyFifoName = emptyFifoName;
    setInterruptOrTermHandler();

    makeNewFifo(emptyFifoName.c_str());
    makeNewFifo(fullFifoName.c_str());
    initShMem();
}


Sigrind::~Sigrind()
{
    /* disconnect from Valgrind */
    munmap(shared, sizeof(SigrindSharedData));
    close(emptyfd);
    close(fullfd);

    /* file cleanup */
    if (remove(shmemName.c_str())     != 0 ||
        remove(emptyFifoName.c_str()) != 0 ||
        remove(fullFifoName.c_str())  != 0 ||
        remove(ipcDir.c_str())        != 0)
    {
        warn(std::string("deleting IPC files -- ") + strerror(errno));
    }
}


auto Sigrind::initShMem() -> void
{
    std::unique_ptr<SigrindSharedData> init(new SigrindSharedData());

    FILE *fd = fopen(shmemName.c_str(), "wb+");

    if (fd == nullptr)
        fatal(std::string("sigrind shared memory file open failed -- ") + strerror(errno));

    /* XXX From write(2) man pages:
     *
     * On Linux, write() (and similar system calls) will transfer at most
     * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
     * actually transferred.  (This is true on both 32-bit and 64-bit
     * systems.)
     *
     * fwrite doesn't have this limitation */
    int count = fwrite(init.get(), sizeof(SigrindSharedData), 1, fd);

    if (count != 1)
    {
        fclose(fd);
        fatal(std::string("sigrind shared memory file write failed -- ") + strerror(errno));
    }

    shared = reinterpret_cast<SigrindSharedData *>
                 (mmap(nullptr, sizeof(SigrindSharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fd), 0));

    if (shared == (void *) - 1)
    {
        fclose(fd);
        fatal(std::string("sigrind mmap shared memory failed -- ") + strerror(errno));
    }

    fclose(fd);
}


auto Sigrind::makeNewFifo(const char *path) const -> void
{
    if (mkfifo(path, 0600) < 0)
    {
        if (errno == EEXIST)
        {
            if (remove(path) != 0)
                fatal(std::string("sigil2 could not delete old fifos -- ") + strerror(errno));

            if (mkfifo(path, 0600) < 0)
                fatal(std::string("sigil2 failed to create valgrind fifos -- ") + strerror(errno));
        }
        else
        {
            fatal(std::string("sigil2 failed to create valgrind fifos -- ") + strerror(errno));
        }
    }
}


auto Sigrind::connectValgrind() -> void
{
    constexpr unsigned max_tries = 8;
    unsigned tries = 0;

    do
    {
        emptyfd = open(emptyFifoName.c_str(), O_WRONLY | O_NONBLOCK);

        if (emptyfd < 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        else //connected
            break;

        ++tries;
    }
    while (tries < max_tries);

    if (tries == max_tries || emptyfd < 0)
        fatal("sigil2 failed to connect to valgrind");

    /* XXX Sigil might get stuck blocking if Valgrind
     * unexpectedly exits before connecting at this point */
    fullfd = open(fullFifoName.c_str(), O_RDONLY);

    if (fullfd < 0)
        fatal(std::string("sigil2 failed to open valgrind fifos -- ") + strerror(errno));
}


auto Sigrind::readFullFifo() -> int
{
    int full_data;
    int res = read(fullfd, &full_data, sizeof(full_data));

    if (res == 0)
        fatal("Unexpected end of fifo");
    else if (res < 0)
        fatal(std::string("could not read from valgrind full fifo -- ") + strerror(errno));

    return full_data;
}


auto Sigrind::writeEmptyFifo(unsigned idx) -> void
{
    auto res = write(emptyfd, &idx, sizeof(idx));

    if (res < 0)
        fatal(std::string("could not send valgrind empty buffer status -- ") + strerror(errno));
}


auto Sigrind::readSigrindEvents() -> void
{
    /* Valgrind should have started by now */
    connectValgrind();

    while (finished == false)
    {
        /* Valgrind sends event buffer metadata */
        unsigned fromVG = readFullFifo();
        emptied.P();

        unsigned idx;
        unsigned used;

        if (fromVG == SIGRIND_FINISHED)
        {
            /* Partial buffer possible */
            finished = true;
            idx = readFullFifo();
            used = readFullFifo();
        }
        else
        {
            /* full buffer */
            idx = fromVG;
            used = MAX_EVENTS;
        }

        q.enqueue(idx);
        filled.V();
    }
}


auto Sigrind::acquire() -> EventBuffer*
{
    filled.P();
    lastBufferIdx = q.dequeue();
    return &(shared->sigrind_buf[lastBufferIdx]);
}


auto Sigrind::release() -> void
{
    emptied.V();

    /* Tell Valgrind that the buffer is empty again */
    writeEmptyFifo(lastBufferIdx);

    /* check if all buffers have been consumed */
    if (finished == true && emptied.value() == NUM_BUFFERS)
    {
        _sigrindReady = false;
        complete.V();
    }
}

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


auto configureWrapperEnv(std::string sigil2_path) -> void
{
    /* check if function capture is available
     * (for multithreaded lib intercepts) */
    std::string sglwrapper(sigil2_path.append("/libsglwrapper.so"));
    std::ifstream sofile(sglwrapper);

    if (sofile.good() == true)
    {
        const char *get_preload = getenv("LD_PRELOAD");
        std::string set_preload;

        if (get_preload == nullptr)
            set_preload = sglwrapper;
        else
            set_preload = std::string(get_preload).append(":").append(sglwrapper);

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


auto tokenizeOpts(const std::vector<std::string>& userExec,
                  const std::vector<std::string>& args,
                  const std::string& ipcDir) -> ExecArgs
{
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

    vg_opts[i++] = strdup((std::string("--ipc-dir=").append(ipcDir)).c_str());

    /*sigrind defaults*/
    vg_opts[i++] = strdup("--gen-mem=yes");
    vg_opts[i++] = strdup("--gen-comp=yes");
    vg_opts[i++] = strdup("--gen-cf=no");
    vg_opts[i++] = strdup("--gen-sync=yes");
    vg_opts[i++] = strdup("--gen-instr=yes");
    vg_opts[i++] = strdup("--gen-bb=no");
    vg_opts[i++] = strdup("--gen-fn=no");

    for (auto &arg : args)
        vg_opts[i++] = strdup(arg.c_str());

    for (auto &arg : userExec)
        vg_opts[i++] = strdup(arg.c_str());

    vg_opts[i] = nullptr;

    return vg_opts;
}


auto configureValgrind(const std::vector<std::string>& userExec,
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

    /* set up valgrind function wrapping to capture synchronization */
    configureWrapperEnv(path);

    /* XXX HACK if the user decides to move the install folder, valgrind will
     * get confused and require that VALGRIND_LIB be set.  Set this variable for
     * the user to avoid confusion */
    setenv("VALGRIND_LIB", std::string(path).append("/vg/lib/valgrind").c_str(), true);

    std::string vg_exec = std::string(path).append("/vg/bin/valgrind");

    /* execvp() expects a const char* const* */
    auto vg_opts = tokenizeOpts(userExec, args, ipcDir);

    return std::make_pair(vg_exec, vg_opts);
}


auto configureShmemPath() -> std::string
{
    /* check IPC path */
    char *shm_path = strdup(getenv("SIGIL2_SHM_DIR") != nullptr ?
                            getenv("SIGIL2_SHM_DIR") : "/dev/shm");

    struct stat info;
    if (stat(shm_path, &info) != 0)
        fatal(std::string(shm_path) + " not found\n" +
              "\tset environment var 'SIGIL2_SHM_DIR' to a tmpfs mount");

    size_t template_length = strlen(shm_path) + sizeof(DIR_TEMPLATE);
    char shm_template[template_length];
    snprintf(shm_template, template_length, "%s%s", shm_path, DIR_TEMPLATE);
    delete[] shm_path;

    if (mkdtemp(shm_template) == nullptr)
        fatal(std::string("creating shm dir failed -- ") + strerror(errno));

    return shm_template;
}
}; //end namespace


////////////////////////////////////////////////////////////
// Interface to Sigil2 core
////////////////////////////////////////////////////////////
auto startSigrind(FrontendStarterArgs args) -> void
{
    const auto& userExecArgs = std::get<0>(args);
    const auto& sigrindArgs  = std::get<1>(args);
    const auto& numThreads   = std::get<2>(args);

    if (numThreads != 1)
        fatal("Valgrind frontend attempted with other than 1 thread");

    gccWarn(userExecArgs);

    std::string shmemPath{configureShmemPath()};

    auto pid = fork();
    if (pid >= 0)
    {
        if (pid == 0)
        {
            auto valgrindArgs = configureValgrind(userExecArgs, sigrindArgs, shmemPath);
            int res = execvp(valgrindArgs.first.c_str(), valgrindArgs.second);

            if (res == -1)
                fatal(std::string("starting valgrind failed -- ") + strerror(errno));
        }
        else
        {
            sigrindIface = std::make_shared<Sigrind>(numThreads, shmemPath);
            _sigrindReady = true;
            sigrindIface->readSigrindEvents();

            /* wait for Sigil2 to consume all buffers */
            complete.P();

            /* teardown the valgrind interface */
            sigrindIface.reset();
        }
    }
    else
    {
        fatal(std::string("sigrind fork failed -- ") + strerror(errno));
    }
}


auto sigrindReady() -> bool
{
    return _sigrindReady;
}


auto acqBufferFromSigrind(unsigned idx) -> const EventBuffer*
{
    /* atomic_load(shared_ptr*) doesn't appear implemented in gcc4.8 */
    // assert(std::atomic_load(&sigrindIface));
    if (idx != 0)
        fatal("Valgrind frontend only has buffer index: 0");

    if (sigrindReady() == false)
        return nullptr;
    else
        return sigrindIface->acquire();
}


auto relBufferFromSigrind(unsigned idx) -> void
{
    /* atomic_load(shared_ptr*) doesn't appear implemented in gcc4.8 */
    // assert(std::atomic_load(&sigrindIface));
    if (idx != 0)
        fatal("Valgrind frontend only has buffer index: 0");

    sigrindIface->release();
}

namespace
{
void sigrindHandler(int s)
{
    /* file cleanup */
    remove(shadow_shmemName.c_str());
    remove(shadow_emptyFifoName.c_str());
    remove(shadow_fullFifoName.c_str());
    remove(shadow_ipcDir.c_str());

    /* set default and re-raise */
    signal(s, SIG_DFL);
    raise(s);
}
};

void Sigrind::setInterruptOrTermHandler()
{
    struct sigaction sig_handler;
    sig_handler.sa_handler = sigrindHandler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, NULL);
    sigaction(SIGTERM, &sig_handler, NULL);
}

}; //end namespace sgl
