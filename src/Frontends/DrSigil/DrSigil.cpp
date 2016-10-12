#include <memory>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <glob.h>
#include <ftw.h>
#include <iostream>

#include "whereami.h"

#include "Sigil2/Sigil.hpp"
#include "DrSigil.hpp"


#define DIR_TEMPLATE "/sgl2-dr-XXXXXX"

/* Sigil2's DynamoRIO frontend forks DynamoRIO off as a separate process;
 * The DynamoRIO client sends the frontend dynamic events from the
 * application via shared memory */

namespace sgl
{

/* TODO rename */
namespace {IPCcleanup ipc_cleanup;};

////////////////////////////////////////////////////////////
// Sigil2 - DynamoRIO IPC
////////////////////////////////////////////////////////////
DrSigil::DrSigil(int ipc_idx, const std::string &ipc_dir)
    : ipc_idx(ipc_idx)
    , curr_thread_id(-1)
    , shmem_file(ipc_dir + "/" + DRSIGIL_SHMEM_NAME + "-" + std::to_string(ipc_idx))
    , empty_file(ipc_dir + "/" + DRSIGIL_EMPTYFIFO_NAME + "-" + std::to_string(ipc_idx))
    , full_file(ipc_dir + "/" + DRSIGIL_FULLFIFO_NAME + "-" + std::to_string(ipc_idx))
{
    ipc_cleanup.addShm(shmem_file);
    ipc_cleanup.addNamedPipe(empty_file);
    ipc_cleanup.addNamedPipe(full_file);

    initShMem();
    makeNewFifo(empty_file.c_str());
    makeNewFifo(full_file.c_str());
}


void DrSigil::initShMem()
{
    std::unique_ptr<DrSigilSharedData> init(new DrSigilSharedData());

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
    int count = fwrite(init.get(), sizeof(DrSigilSharedData), 1, fd);

    if (count != 1)
    {
        fclose(fd);
        SigiLog::fatal(std::string("sigrind shared memory file write failed -- ").append(strerror(errno)));
    }

    shared_mem = reinterpret_cast<DrSigilSharedData *>
                 (mmap(nullptr, sizeof(DrSigilSharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fd), 0));

    if (shared_mem == (void *)-1)
    {
        fclose(fd);
        SigiLog::fatal(std::string("sigrind mmap shared memory failed -- ").append(strerror(errno)));
    }

    fclose(fd);
}


void DrSigil::makeNewFifo(const char *path) const
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
                SigiLog::fatal(std::string("sigil2 failed to create dynamorio fifos -- ").append(strerror(errno)));
            }
        }
        else
        {
            SigiLog::fatal(std::string("sigil2 failed to create dynamorio fifos -- ").append(strerror(errno)));
        }
    }
}


void DrSigil::connectDynamoRIO()
{
    /* Indefinitely wait for DynamoRIO to connect
     * because DR will not connect until a corresponding
     * thread is spawned for event generation.
     *
     * The event generation thread spawn time is undefined */
    emptyfd = open(empty_file.c_str(), O_WRONLY);

    /* XXX Sigil might get stuck blocking if DynamoRIO
     * unexpectedly exits before connecting at this point */
    fullfd = open(full_file.c_str(), O_RDONLY);

    if (fullfd < 0)
    {
        SigiLog::fatal(std::string("sigil2 failed to open dynamorio fifos -- ").append(strerror(errno)));
    }

    ipc_cleanup.addPipeFD(emptyfd);
    ipc_cleanup.addPipeFD(fullfd);
}


void DrSigil::disconnectDynamoRIO()
{
    munmap(shared_mem, sizeof(DrSigilSharedData));
    close(emptyfd);
    close(fullfd);
}


int DrSigil::readFullFifo()
{
    int res = read(fullfd, &full_data, sizeof(full_data));

    if (res == 0)
    {
        SigiLog::fatal("Unexpected end of fifo");
    }
    else if (res < 0)
    {
        SigiLog::fatal(std::string("could not read from dynamorio full fifo -- ").append(strerror(errno)));
    }

    return full_data;
}


void DrSigil::writeEmptyFifo(unsigned int idx)
{
    if (write(emptyfd, &idx, sizeof(idx)) < 0)
    {
        SigiLog::fatal(std::string("could not send dynamorio empty buffer status -- ").append(strerror(
                           errno)));
    }
}


void DrSigil::produceFromBuffer(unsigned int idx, unsigned int used)
{
    assert(idx < DRSIGIL_BUFNUM);

    DrSigilEvent(&buf)[DRSIGIL_BUFSIZE] = shared_mem->buf[idx];

    for (unsigned int i = 0; i < used; ++i)
    {
        /* Sigil2 backend indices start at '0' */
        assert(ipc_idx == (buf[i].thread_id) % num_threads);

        if (ipc_idx != (buf[i].thread_id) % num_threads)
        {
            SigiLog::fatal("IPC channel received incorrect thread");
        }

        /* The thread id is checked for every event,
         * because there is no restriction on event
         * ordering between different threads in DynamoRIO
         *
         * Thread switch events are generated here,
         * instead of in DynamorRIO */
        if (buf[i].thread_id != curr_thread_id)
        {
            curr_thread_id = buf[i].thread_id;
            thread_swap_event.type = SGLPRIM_SYNC_SWAP;
            thread_swap_event.id = curr_thread_id;
            Sigil::instance().addEvent(thread_swap_event, ipc_idx);
        }

        switch (buf[i].ev.tag)
        {
        case EvTagEnum::SGL_MEM_TAG:
            Sigil::instance().addEvent(buf[i].ev.mem, ipc_idx);
            break;

        case EvTagEnum::SGL_COMP_TAG:
            Sigil::instance().addEvent(buf[i].ev.comp, ipc_idx);
            break;

        case EvTagEnum::SGL_SYNC_TAG:
            Sigil::instance().addEvent(buf[i].ev.sync, ipc_idx);
            break;

        case EvTagEnum::SGL_CXT_TAG:
            Sigil::instance().addEvent(buf[i].ev.cxt, ipc_idx);
            break;

        default:
            SigiLog::fatal("received unhandled event in sigrind");
            break;
        }
    }
}

void DrSigil::produceDynamoRIOEvents()
{
    /* DynamoRIO should have initialized by now */
    connectDynamoRIO();

    while (finished == false)
    {
        /* Each DynamoRIO thread sends event buffer metadata */
        unsigned int from_dynamorio = readFullFifo();

        unsigned int idx;
        unsigned int used;

        if (from_dynamorio == DRSIGIL_FINISHED)
        {
            std::cerr << "DrSigil finish detected for IPC " << ipc_idx << std::endl;
            /* DynamoRIO thread finished;
             * partial leftover buffer */
            finished = true;
            idx = readFullFifo();
            used = readFullFifo();
        }
        else
        {
            /* full buffer */
            used = from_dynamorio;
            idx = readFullFifo();
        }

        /* send data to backend */
        produceFromBuffer(idx, used);

        /* tell DynamoRIO that the buffer is empty again */
        writeEmptyFifo(idx);
    }

    /* lets DynamoRIO know Sigil is finished */
    std::cerr << "DrSigil disconnecting IPC " << ipc_idx << std::endl;
    disconnectDynamoRIO();
}


////////////////////////////////////////////////////////////
// Launching DynamoRIO
////////////////////////////////////////////////////////////
namespace
{
using ExecArgs = char *const *;
using Exec = std::pair<std::string, ExecArgs>;

ExecArgs tokenizeOpts(const std::vector<std::string> &user_exec,
                      const std::vector<std::string> &args,
                      const std::string &sigil_bin_dir,
                      const std::string &ipc_dir,
                      const uint16_t num_threads)
{
    assert(!user_exec.empty() && !ipc_dir.empty());

    /* format dynamorio options */
    //                 program name + dynamorio options + user program options + null
    int dr_opts_size = 1            + 8 + args.size()     + user_exec.size()        + 1;
    char **dr_opts = static_cast<char **>(malloc(dr_opts_size * sizeof(char *)));

    int i = 0;
    dr_opts[i++] = strdup("drrun");
    dr_opts[i++] = strdup("-root");
    dr_opts[i++] = strdup((sigil_bin_dir + "/dr").c_str());
    dr_opts[i++] = strdup("-c");

    /* detect 32/64 bit and release/debug build */
    glob_t glob_result;
    glob(std::string(sigil_bin_dir).append("/dr/tools/lib*/*").c_str(),
         GLOB_MARK|GLOB_TILDE|GLOB_ONLYDIR,
         NULL, &glob_result);

    if (glob_result.gl_pathc != 1)
    {
        SigiLog::fatal("Error detecting \'libdrsigil.so\' path");
    }

    std::string dr_lib = std::string(glob_result.gl_pathv[0]);

    globfree(&glob_result);

    dr_opts[i++] = strdup((dr_lib + "libdrsigil.so").c_str());
    dr_opts[i++] = strdup(("--num-frontend-threads=" + std::to_string(num_threads)).c_str());
    dr_opts[i++] = strdup(("--ipc-dir=" + ipc_dir).c_str());

    for (auto &arg : args)
    {
        dr_opts[i++] = strdup(arg.c_str());
    }

    dr_opts[i++] = strdup("--");

    for (auto &arg : user_exec)
    {
        dr_opts[i++] = strdup(arg.c_str());
    }

    dr_opts[i] = nullptr;

    return dr_opts;
}


Exec configureDynamoRIO(const std::vector<std::string> &user_exec,
                        const std::vector<std::string> &args,
                        const std::string &ipc_dir,
                        const uint16_t num_threads)
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

    /* detect 32/64 bit */
    glob_t glob_result;
    glob(std::string(path).append("/dr/bin*").c_str(),
         GLOB_MARK|GLOB_TILDE|GLOB_ONLYDIR,
         NULL, &glob_result);

    if (glob_result.gl_pathc != 1)
    {
        SigiLog::fatal("Error detecting \'drrun\' path");
    }

    std::string dr_exec = std::string(glob_result.gl_pathv[0]) + ("drrun");

    globfree(&glob_result);

    /* execvp() expects a const char* const* */
    auto dr_opts = tokenizeOpts(user_exec, args, path, ipc_dir, num_threads);

    return std::make_pair(dr_exec, dr_opts);
}

}; //end namespace


/* clean up IPC files */
namespace
{
void drsigilCleanupHandler(int s)
{
    /* file cleanup */
    ipc_cleanup.cleanup();

    /* set default and re-raise */
    signal(s, SIG_DFL);
    raise(s);
}

void setInterruptOrTermHandler()
{
    struct sigaction sig_handler;
    sig_handler.sa_handler = drsigilCleanupHandler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, NULL);
    sigaction(SIGTERM, &sig_handler, NULL);
}
};

/* static init */
int sgl::DrSigil::num_threads = 1;

void DrSigil::start(const std::vector<std::string> &user_exec,
                    const std::vector<std::string> &args,
                    const uint16_t num_threads,
                    const std::string &instance_id)
{
    assert(user_exec.empty() == false);

    DrSigil::num_threads = num_threads;

    /* check IPC path */
    char *shm_path = getenv("SIGIL2_SHM_DIR");

    /* posix shmem typically uses /dev/shm */
    if (shm_path == nullptr)
    {
        shm_path = strdup("/dev/shm");
    }

    struct stat info;
    if (stat(shm_path, &info) != 0)
    {
        SigiLog::fatal(std::string(shm_path) +
                       " not found\n\t"
                       "set environment var 'SIGIL2_SHM_DIR' to a tmpfs mount");
    }

    size_t template_length = strlen(shm_path) + sizeof(DIR_TEMPLATE);
    char shm_template[template_length];
    snprintf(shm_template, template_length, "%s%s", shm_path, DIR_TEMPLATE);

    if (mkdtemp(shm_template) == nullptr)
    {
        SigiLog::fatal(std::string("creating shm dir failed -- ").append(strerror(errno)));
    }

    /* clean up */
    ipc_cleanup.ipc_dir = shm_template;
    setInterruptOrTermHandler();

    /* set up dynamorio environment */
    auto dynamorio_args = configureDynamoRIO(user_exec, args, shm_template, num_threads);

    /* DynamoRIO frontend has a multithreaded interface */
    std::vector<std::shared_ptr<DrSigil>> dr_ifaces;
    std::vector<std::thread> dr_iface_producers;

    /* set up interfaces to dynamorio */
    for (int i = 0; i < num_threads; ++i)
    {
        dr_ifaces.push_back(std::make_shared<DrSigil>(i, +shm_template));
    }

    pid_t pid = fork();

    if (pid >= 0)
    {
        if (pid == 0)
        {
            /* kickoff DynamoRIO */
            int res = execvp(dynamorio_args.first.c_str(), dynamorio_args.second);

            if (res == -1)
            {
                SigiLog::fatal(std::string("starting dynamorio failed -- ").append(strerror(errno)));
            }
        }
        else
        {
            for (auto &iface : dr_ifaces)
            {
                dr_iface_producers.push_back(std::thread(&DrSigil::produceDynamoRIOEvents, iface.get()));
            }

            for (auto &thr : dr_iface_producers)
            {
                thr.join();
            }
        }
    }
    else
    {
        SigiLog::fatal(std::string("sigrind fork failed -- ").append(strerror(errno)));
    }
    
    ipc_cleanup.cleanup();
}


void IPCcleanup::cleanup()
{
    for (auto fd : fds)
    {
        close(fd);
    }

    for (auto &pipe : named_pipes)
    {
        std::remove(pipe.c_str());
    }

    for (auto &shm : shm_files)
    {
        std::remove(shm.c_str());
    }

    std::remove(ipc_dir.c_str());
}


void IPCcleanup::addNamedPipe(const std::string &pipe)
{
    std::lock_guard<std::mutex> lock(m);
    named_pipes.emplace_back(pipe);
}


void IPCcleanup::addPipeFD(int fd)
{
    std::lock_guard<std::mutex> lock(m);
    fds.emplace_back(fd);
}


void IPCcleanup::addShm(const std::string &shm)
{
    std::lock_guard<std::mutex> lock(m);
    shm_files.emplace_back(shm);
}

}; //end namespace sgl
