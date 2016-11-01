#include <iterator>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "Sigil2/Sigil.hpp"
#include "Sigil2/InstrumentationIface.h"
#include "Sigrind.hpp"
#include "whereami.h"
#include "elfio/elfio.hpp"

namespace sgl
{

namespace
{
/* signal handler needs this to know which files to clean up */
std::string shadow_shmem_file;
std::string shadow_empty_file;
std::string shadow_full_file;
};

////////////////////////////////////////////////////////////
// Sigil2 - Valgrind IPC
////////////////////////////////////////////////////////////
Sigrind::Sigrind(int num_threads, const std::string &tmp_dir, const std::string &instance_id)
    : timestamp(instance_id)
    , shmem_file(tmp_dir + "/" + SIGRIND_SHMEM_NAME + timestamp)
    , empty_file(tmp_dir + "/" + SIGRIND_EMPTYFIFO_NAME + timestamp)
    , full_file(tmp_dir + "/" + SIGRIND_FULLFIFO_NAME + timestamp)
    , num_threads(num_threads)
    , be_idx(0)
{
    shadow_shmem_file = shmem_file;
    shadow_full_file  = full_file;
    shadow_empty_file = empty_file;

    setInterruptOrTermHandler();
    initShMem();
    makeNewFifo(empty_file.c_str());
    makeNewFifo(full_file.c_str());
}


Sigrind::~Sigrind()
{
    /* disconnect from Valgrind */
    munmap(shared_mem, sizeof(SigrindSharedData));
    close(emptyfd);
    close(fullfd);

    /* file cleanup */
    if (remove(shmem_file.c_str()) != 0 ||
        remove(empty_file.c_str()) != 0 ||
        remove(full_file.c_str()) != 0)
    {
        SigiLog::warn(std::string("deleting IPC files -- ").append(strerror(errno)));
    }
}

void Sigrind::initShMem()
{
    std::unique_ptr<SigrindSharedData> init(new SigrindSharedData());

    FILE *fd = fopen(shmem_file.c_str(), "wb+");

    if (fd == nullptr)
    {
        SigiLog::fatal(std::string("sigrind shared memory file open failed -- ").append(strerror(errno)));
    }

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
        SigiLog::fatal(std::string("sigrind shared memory file write failed -- ").append(strerror(errno)));
    }

    shared_mem = reinterpret_cast<SigrindSharedData *>
                 (mmap(nullptr, sizeof(SigrindSharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fd), 0));

    if (shared_mem == (void *) - 1)
    {
        fclose(fd);
        SigiLog::fatal(std::string("sigrind mmap shared memory failed -- ").append(strerror(errno)));
    }

    fclose(fd);
}


void Sigrind::makeNewFifo(const char *path) const
{
    if (mkfifo(path, 0600) < 0)
    {
        if (errno == EEXIST)
        {
            if (remove(path) != 0)
            {
                SigiLog::fatal(std::string("sigil2 could not delete old fifos -- ").append(strerror(errno)));
            }

            if (mkfifo(path, 0600) < 0)
            {
                SigiLog::fatal(std::string("sigil2 failed to create valgrind fifos -- ").append(strerror(errno)));
            }
        }
        else
        {
            SigiLog::fatal(std::string("sigil2 failed to create valgrind fifos -- ").append(strerror(errno)));
        }
    }
}


void Sigrind::connectValgrind()
{
    int tries = 0;

    do
    {
        emptyfd = open(empty_file.c_str(), O_WRONLY | O_NONBLOCK);

        if (emptyfd < 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        else //connected
        {
            break;
        }

        ++tries;
    }
    while (tries < 4);

    if (tries == 4 || emptyfd < 0)
    {
        SigiLog::fatal("sigil2 failed to connect to valgrind");
    }

    /* XXX Sigil might get stuck blocking if Valgrind
     * unexpectedly exits before connecting at this point */
    fullfd = open(full_file.c_str(), O_RDONLY);

    if (fullfd < 0)
    {
        SigiLog::fatal(std::string("sigil2 failed to open valgrind fifos -- ").append(strerror(errno)));
    }
}


int Sigrind::readFullFifo()
{
    int res = read(fullfd, &full_data, sizeof(full_data));

    if (res == 0)
    {
        SigiLog::fatal("Unexpected end of fifo");
    }
    else if (res < 0)
    {
        SigiLog::fatal(std::string("could not read from valgrind full fifo -- ").append(strerror(errno)));
    }

    return full_data;
}


void Sigrind::writeEmptyFifo(unsigned int idx)
{
    if (write(emptyfd, &idx, sizeof(idx)) < 0)
    {
        SigiLog::fatal(std::string("could not send valgrind empty buffer status -- ").append(strerror(errno)));
    }
}


#ifdef SIGRIND_DEBUG
unsigned long long mem_events;
unsigned long long comp_events;
unsigned long long sync_events;
unsigned long long cxt_events;
#endif

void Sigrind::produceFromBuffer(unsigned int idx, unsigned int used)
{
    assert(idx < SIGRIND_BUFNUM);

    BufferedSglEv(&buf)[SIGRIND_BUFSIZE] = shared_mem->buf[idx];

    for (unsigned int i = 0; i < used; ++i)
    {
        switch (buf[i].tag)
        {
        case EvTagEnum::SGL_MEM_TAG:
            Sigil::instance().addEvent(buf[i].mem, be_idx);
            break;

        case EvTagEnum::SGL_COMP_TAG:
            Sigil::instance().addEvent(buf[i].comp, be_idx);
            break;

        case EvTagEnum::SGL_CF_TAG:
            Sigil::instance().addEvent(buf[i].cf, be_idx);
            break;

        case EvTagEnum::SGL_SYNC_TAG:
            Sigil::instance().addEvent(buf[i].sync, be_idx);
            break;

        case EvTagEnum::SGL_CXT_TAG:
            Sigil::instance().addEvent(buf[i].cxt, be_idx);
            break;

        default:
            SigiLog::fatal("received unhandled event in sigrind: " + std::to_string(buf[i].tag));
            break;
        }

#ifdef SIGRIND_DEBUG
        switch (buf[i].tag)
        {
        case EvTagEnum::SGL_MEM_TAG:
            ++mem_events;
            break;
        case EvTagEnum::SGL_COMP_TAG:
            ++comp_events;
            break;
        case EvTagEnum::SGL_CF_TAG:
            break;
        case EvTagEnum::SGL_SYNC_TAG:
            ++sync_events;
            break;
        case EvTagEnum::SGL_CXT_TAG:
            ++cxt_events;
            break;
        default:
            break;
        }
#endif

    }
}


void Sigrind::produceSigrindEvents()
{
    /* Valgrind should have started by now */
    connectValgrind();

    while (finished == false)
    {
        /* Valgrind sends event buffer metadata */
        unsigned int from_valgrind = readFullFifo();

        unsigned int idx;
        unsigned int used;

        if (from_valgrind == SIGRIND_FINISHED)
        {
            /* Valgrind finished;
             * partial leftover buffer */
            finished = true;
            idx = readFullFifo();
            used = readFullFifo();
        }
        else
        {
            /* full buffer */
            idx = from_valgrind;
            used = SIGRIND_BUFSIZE;
        }

        /* send data to backend */
        produceFromBuffer(idx, used);

        /* tell Valgrind that the buffer is empty again */
        writeEmptyFifo(idx);
    }

#ifdef SIGRIND_DEBUG
    SigiLog::info("Memory Events: "  + std::to_string(mem_events));
    SigiLog::info("Compute Events: " + std::to_string(comp_events));
    SigiLog::info("Sync Events: "    + std::to_string(sync_events));
    SigiLog::info("Context Events: " + std::to_string(cxt_events));
#endif
}


////////////////////////////////////////////////////////////
// Launching Valgrind
////////////////////////////////////////////////////////////
namespace
{
using ExecArgs = char *const *;
using Exec = std::pair<std::string, ExecArgs>;

void gccWarn(const std::vector<std::string> &user_exec)
{
    assert(user_exec.empty() == false);

    /* Naively assume the first option is the user binary.
     * ML: KS says that OpenMP is only guaranteed to work for
     * GCC 4.9.2. Pthreads should work for most recent GCC
     * releases. Cannot check if file exists because it is
     * not guaranteed that this string is actually the binary */
    ELFIO::elfio reader;
    bool is_gcc_compatible = false;
    std::string gcc_version_needed("4.9.2");
    std::string gcc_version_found;

    if (reader.load(user_exec[0]) != 0)
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
        SigiLog::warn("\'" + user_exec[0] + "\'" + ":");
        SigiLog::warn("GCC version " + gcc_version_needed + " not detected");

        if (gcc_version_found.empty() == false)
        {
            SigiLog::warn("GCC version " + gcc_version_found + " found");
        }
        else
        {
            SigiLog::warn("GCC version could not be detected");
        }

        SigiLog::warn("OpenMP synchronization events may not be captured");
        SigiLog::warn("Pthread synchronization events are probably fine");
    }
}


void configureWrapperEnv(std::string sigil2_path)
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
        {
            set_preload = sglwrapper;
        }
        else
        {
            set_preload = std::string(get_preload).append(":").append(sglwrapper);
        }

        setenv("LD_PRELOAD", set_preload.c_str(), true);
    }
    else
    {
        /* If the wrapper library is in LD_PRELOAD,
         * but not in the sigil2 directory,
         * then this warning can be ignored */
        SigiLog::warn("'sglwrapper.so' not found");
        SigiLog::warn("Synchronization events will not be detected without the wrapper library loaded");
    }

    sofile.close();
}


ExecArgs tokenizeOpts(const std::vector<std::string> &user_exec,
                      const std::vector<std::string> &args,
                      const std::string &tmp_dir,
                      const std::string &timestamp)
{
    assert(!user_exec.empty() && !tmp_dir.empty());

    /* format valgrind options */
    int vg_opts_size = 1/*program name*/ +
                       2/*vg opts*/ +
                       1/*tmp_dir*/ +
                       1/*timestamp*/ +
                       8/*sigrind opts default*/ +
                       args.size()/*sigrind opts defined*/ +
                       user_exec.size()/*user program options*/ +
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

    vg_opts[i++] = strdup((std::string("--tmp-dir=").append(tmp_dir)).c_str());
    vg_opts[i++] = strdup((std::string("--timestamp=").append(timestamp)).c_str());

    /*sigrind defaults*/
    vg_opts[i++] = strdup("--gen-mem=yes");
    vg_opts[i++] = strdup("--gen-comp=yes");
    vg_opts[i++] = strdup("--gen-cf=no");
    vg_opts[i++] = strdup("--gen-sync=yes");
    vg_opts[i++] = strdup("--gen-instr=yes");
    vg_opts[i++] = strdup("--gen-bb=no");
    vg_opts[i++] = strdup("--gen-fn=no");

    for (auto &arg : args)
    {
        vg_opts[i++] = strdup(arg.c_str());
    }

    for (auto &arg : user_exec)
    {
        vg_opts[i++] = strdup(arg.c_str());
    }

    vg_opts[i] = nullptr;

    return vg_opts;
}


Exec configureValgrind(const std::vector<std::string> &user_exec,
                       const std::vector<std::string> &args,
                       const std::string &tmp_path,
                       const std::string &timestamp)
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
        SigiLog::fatal("couldn't find executable path");
    }

    /* set up valgrind function wrapping to capture synchronization */
    configureWrapperEnv(path);

    /* XXX HACK if the user decides to move the install folder, valgrind will
     * get confused and require that VALGRIND_LIB be set.  Set this variable for
     * the user to avoid confusion */
    setenv("VALGRIND_LIB", std::string(path).append("/vg/lib/valgrind").c_str(), true);

    std::string vg_exec = std::string(path).append("/vg/bin/valgrind");

    /* execvp() expects a const char* const* */
    auto vg_opts = tokenizeOpts(user_exec, args, tmp_path, timestamp);

    return std::make_pair(vg_exec, vg_opts);
}
}; //end namespace


void Sigrind::start(const std::vector<std::string> &user_exec,
                    const std::vector<std::string> &args,
                    const uint16_t num_threads,
                    const std::string &instance_id)
{
    assert(user_exec.empty() == false);

    if (num_threads != 1)
    {
        SigiLog::fatal("Valgrind frontend attempted with other than 1 thread");
    }

    /* warn user if gcc version is not fully supported */
    gccWarn(user_exec);

    /* check IPC path */
    char *tmp_path = getenv("TMPDIR");

    /* posix shmem typically uses /dev/shm
     * valgrind API doesn't provide shmem syscalls, manually mmap */
    if (tmp_path == nullptr)
    {
        tmp_path = strdup("/dev/shm");
    }

    struct stat info;

    if (stat(tmp_path, &info) != 0)
    {
        SigiLog::fatal(std::string(tmp_path).append(
                           " not found\n\tset environment var 'TMPDIR' to a tmpfs mount"));
    }

    /* set up interface to valgrind */
    Sigrind sigrind_iface(num_threads, tmp_path, instance_id);

    /* set up valgrind environment */
    auto valgrind_args = configureValgrind(user_exec, args, tmp_path, instance_id);

    pid_t pid = fork();

    if (pid >= 0)
    {
        if (pid == 0)
        {
            /* kickoff Valgrind */
            int res = execvp(valgrind_args.first.c_str(), valgrind_args.second);

            if (res == -1)
            {
                SigiLog::fatal(std::string("starting valgrind failed -- ").append(strerror(errno)));
            }
        }
        else
        {
            sigrind_iface.produceSigrindEvents();
        }
    }
    else
    {
        SigiLog::fatal(std::string("sigrind fork failed -- ").append(strerror(errno)));
    }
}

namespace
{
void sigrindHandler(int s)
{
    /* file cleanup */
    remove(shadow_shmem_file.c_str());
    remove(shadow_empty_file.c_str());
    remove(shadow_full_file.c_str());

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
