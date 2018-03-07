#include "sigil2_ipc.h"
#include "coregrind/pub_core_libcfile.h"
#include "coregrind/pub_core_aspacemgr.h"
#include "coregrind/pub_core_syscall.h"
#include "pub_tool_basics.h"
#include "pub_tool_vki.h"       // errnum, vki_timespec
#include "pub_tool_vkiscnums.h" // __NR_nanosleep

static Bool initialized = False;
static Int emptyfd;
static Int fullfd;
static Sigil2DBISharedData* shmem;
/* IPC channel */


static UInt           curr_idx;
static EventBuffer*   curr_ev_buf;
static SglEvVariant*  curr_ev_slot;
static NameBuffer*    curr_name_buf;
static char*          curr_name_slot;
/* cached IPC state */


static Bool is_full[SIGIL2_IPC_BUFFERS];
/* track available buffers */


static inline void set_and_init_buffer(UInt buf_idx)
{
    curr_ev_buf = shmem->eventBuffers + buf_idx;
    curr_ev_buf->used = 0;
    curr_ev_slot = curr_ev_buf->events + curr_ev_buf->used;

    curr_name_buf = shmem->nameBuffers + buf_idx;
    curr_name_buf->used = 0;
    curr_name_slot = curr_name_buf->names + curr_name_buf->used;
}


static inline void flush_to_sigil2(void)
{
    /* Mark that the buffer is being flushed,
     * and tell Sigil2 the buffer is ready to consume */
    is_full[curr_idx] = True;
    Int res = VG_(write)(fullfd, &curr_idx, sizeof(curr_idx));
    if (res != sizeof(curr_idx))
    {
        VG_(umsg)("error VG_(write)\n");
        VG_(umsg)("error writing to Sigrind fifo\n");
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }
}


static inline void set_next_buffer(void)
{
    /* try the next buffer, circular */
    ++curr_idx;
    if (curr_idx == SIGIL2_IPC_BUFFERS)
        curr_idx = 0;

    /* if the next buffer is full,
     * wait until Sigil2 communicates that it's free */
    if (is_full[curr_idx])
    {
        UInt buf_idx;
        Int res = VG_(read)(emptyfd, &buf_idx, sizeof(buf_idx));
        if (res != sizeof(buf_idx))
        {
            VG_(umsg)("error VG_(read)\n");
            VG_(umsg)("error reading from Sigrind fifo\n");
            VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
            VG_(exit)(1);
        }

        tl_assert(buf_idx < SIGIL2_IPC_BUFFERS);
        tl_assert(buf_idx == curr_idx);
        curr_idx = buf_idx;
        is_full[curr_idx] = False;
    }

    set_and_init_buffer(curr_idx);
}


static inline Bool is_events_full(void)
{
    return curr_ev_buf->used == SIGIL2_EVENTS_BUFFER_SIZE;
}


static inline Bool is_names_full(UInt size)
{
    return (curr_name_buf->used + size) > SIGIL2_EVENTS_BUFFER_SIZE;
}


SglEvVariant* SGL_(acq_event_slot)()
{
    tl_assert(initialized == True);

    if (is_events_full())
    {
        flush_to_sigil2();
        set_next_buffer();
    }

    curr_ev_buf->used++;
    return curr_ev_slot++;
}


EventNameSlotTuple SGL_(acq_event_name_slot)(UInt size)
{
    tl_assert(initialized == True);

    if (is_events_full() || is_names_full(size))
    {
        flush_to_sigil2();
        set_next_buffer();
    }

    EventNameSlotTuple tuple = {curr_ev_slot, curr_name_slot, curr_name_buf->used};
    curr_ev_buf->used   += 1;
    curr_ev_slot        += 1;
    curr_name_buf->used += size;
    curr_name_slot      += size;

    return tuple;
}


/******************************
 * Initialization/Termination
 ******************************/
static int open_fifo(const HChar *fifo_path, int flags)
{
    tl_assert(initialized == False);

    int tries = 0;
    const int max_tries = 4;
    int fd = VG_(fd_open)(fifo_path, flags, 0600);
    while (fd < 0)
    {
        if (++tries < max_tries)
        {
#if defined(VGO_linux) && defined(VGA_amd64)
            /* TODO any serious implications in Valgrind of calling syscalls directly?
             * MDL20170220 The "VG_(syscall)" wrappers don't look like they do much
             * else besides doing platform specific setup.
             * In our case, we only accommodate x86_64 or aarch64. */
            struct vki_timespec req;
            req.tv_sec = 0;
            req.tv_nsec = 500000000;
            /* wait some time before trying to connect,
             * giving Sigil2 time to bring up IPC */
            VG_(do_syscall2)(__NR_nanosleep, (UWord)&req, 0);
#else
#error "Only linux is supported"
#endif
            fd = VG_(fd_open)(fifo_path, flags, 0600);
        }
        else
        {
            VG_(umsg)("FIFO for Sigrind failed\n");
            VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
            VG_(exit) (1);
        }
    }

    return fd;
}


static Sigil2DBISharedData* open_shmem(const HChar *shmem_path, int flags)
{
    tl_assert(initialized == False);

    int shared_mem_fd = VG_(fd_open)(shmem_path, flags, 0600);
    if (shared_mem_fd < 0)
    {
        VG_(umsg)("Cannot open shared_mem file %s\n", shmem_path);
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }

    SysRes res = VG_(am_shared_mmap_file_float_valgrind)(sizeof(Sigil2DBISharedData),
                                                         VKI_PROT_READ|VKI_PROT_WRITE,
                                                         shared_mem_fd, (Off64T)0);
    if (sr_isError(res))
    {
        VG_(umsg)("error %lu %s\n", sr_Err(res), VG_(strerror)(sr_Err(res)));
        VG_(umsg)("error VG_(am_shared_mmap_file_float_valgrind) %s\n", shmem_path);
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }

    Addr addr_shared = sr_Res (res);
    VG_(close)(shared_mem_fd);

    return (Sigil2DBISharedData*) addr_shared;
}


void SGL_(init_IPC)()
{
    tl_assert(initialized == False);

    if (SGL_(clo).ipc_dir == NULL)
    {
       VG_(fmsg)("No --ipc-dir argument found, shutting down...\n");
       VG_(exit)(1);
    }

    Int ipc_dir_len = VG_(strlen)(SGL_(clo).ipc_dir);
    Int filename_len;

    //len is strlen + null + other chars (/ and -0)
    filename_len = ipc_dir_len + VG_(strlen)(SIGIL2_IPC_SHMEM_BASENAME) + 4;
    HChar shmem_path[filename_len];
    VG_(snprintf)(shmem_path, filename_len, "%s/%s-0", SGL_(clo).ipc_dir, SIGIL2_IPC_SHMEM_BASENAME);

    filename_len = ipc_dir_len + VG_(strlen)(SIGIL2_IPC_EMPTYFIFO_BASENAME) + 4;
    HChar emptyfifo_path[filename_len];
    VG_(snprintf)(emptyfifo_path, filename_len, "%s/%s-0", SGL_(clo).ipc_dir, SIGIL2_IPC_EMPTYFIFO_BASENAME);

    filename_len = ipc_dir_len + VG_(strlen)(SIGIL2_IPC_FULLFIFO_BASENAME) + 4;
    HChar fullfifo_path[filename_len];
    VG_(snprintf)(fullfifo_path, filename_len, "%s/%s-0", SGL_(clo).ipc_dir, SIGIL2_IPC_FULLFIFO_BASENAME);

    emptyfd = open_fifo(emptyfifo_path, VKI_O_RDONLY);
    fullfd  = open_fifo(fullfifo_path, VKI_O_WRONLY);
    shmem   = open_shmem(shmem_path, VKI_O_RDWR);

    /* initialize cached IPC state */
    curr_idx = 0;
    set_and_init_buffer(curr_idx);
    for (UInt i=0; i<SIGIL2_IPC_BUFFERS; ++i)
        is_full[i] = False;

    initialized = True;
}


void SGL_(term_IPC)(void)
{
    tl_assert(initialized == True);

    /* send finish sequence */
    UInt finished = SIGIL2_IPC_FINISHED;
    if (VG_(write)(fullfd, &curr_idx, sizeof(curr_idx)) != sizeof(curr_idx) ||
        VG_(write)(fullfd, &finished, sizeof(finished)) != sizeof(finished))
    {
        VG_(umsg)("error VG_(write)\n");
        VG_(umsg)("error writing to Sigrind fifo\n");
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }

    /* wait until Sigrind disconnects */
    while (VG_(read)(emptyfd, &finished, sizeof(finished)) > 0);

    VG_(close)(emptyfd);
    VG_(close)(fullfd);
}
