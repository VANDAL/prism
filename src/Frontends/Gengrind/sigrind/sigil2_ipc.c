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
static PrismDBISharedData* shmem;
/* IPC channel */


static uint32_t        curr_ev_buf_offset;
static unsigned char*  curr_ev_buf_baseptr;
static UInt            ipc_ev_buf_idx;
static EventBuffer*    ipc_ev_buf;
/* cached IPC state */


static Bool ipc_buf_is_full[PRISM_IPC_BUFFERS];
/* track available buffers */


static inline void set_and_init_buffer(UInt buf_idx)
{
    ipc_ev_buf = shmem->eventBuffers + buf_idx;
    curr_ev_buf_baseptr = ipc_ev_buf->events;
    curr_ev_buf_offset = 0;
}


static inline void flush_to_prism(void)
{
    /* Mark that the buffer is being flushed,
     * and tell Prism the buffer is ready to consume */
    unsigned char* buf = curr_ev_buf_baseptr + curr_ev_buf_offset;
    SET_EV_END(buf);

    ipc_buf_is_full[ipc_ev_buf_idx] = True;
    Int res = VG_(write)(fullfd, &ipc_ev_buf_idx, sizeof(ipc_ev_buf_idx));
    if (res != sizeof(ipc_ev_buf_idx))
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
    ++ipc_ev_buf_idx;
    if (ipc_ev_buf_idx == PRISM_IPC_BUFFERS)
        ipc_ev_buf_idx = 0;

    /* if the next buffer is full,
     * wait until Prism communicates that it's free */
    if (ipc_buf_is_full[ipc_ev_buf_idx])
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

        tl_assert(buf_idx < PRISM_IPC_BUFFERS);
        tl_assert(buf_idx == ipc_ev_buf_idx);
        ipc_ev_buf_idx = buf_idx;
        ipc_buf_is_full[ipc_ev_buf_idx] = False;
    }

    set_and_init_buffer(ipc_ev_buf_idx);
}


unsigned char* SGL_(reserve_ev_buf)(uint32_t bytes_requested)
{
    tl_assert(initialized == True);

    if ((curr_ev_buf_offset + bytes_requested) >= PRISM_EVENTS_BUFFER_SIZE)
    {
        flush_to_prism();
        set_next_buffer();
    }

    unsigned char* temp = curr_ev_buf_baseptr + curr_ev_buf_offset;
    curr_ev_buf_offset += bytes_requested;
    return temp;
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
             * giving Prism time to bring up IPC */
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


static PrismDBISharedData* open_shmem(const HChar *shmem_path, int flags)
{
    tl_assert(initialized == False);

    int shared_mem_fd = VG_(fd_open)(shmem_path, flags, 0600);
    if (shared_mem_fd < 0)
    {
        VG_(umsg)("Cannot open shared_mem file %s\n", shmem_path);
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }

    SysRes res = VG_(am_shared_mmap_file_float_valgrind)(sizeof(PrismDBISharedData),
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

    return (PrismDBISharedData*) addr_shared;
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
    filename_len = ipc_dir_len + VG_(strlen)(PRISM_IPC_SHMEM_BASENAME) + 4;
    HChar shmem_path[filename_len];
    VG_(snprintf)(shmem_path, filename_len, "%s/%s-0", SGL_(clo).ipc_dir, PRISM_IPC_SHMEM_BASENAME);

    filename_len = ipc_dir_len + VG_(strlen)(PRISM_IPC_EMPTYFIFO_BASENAME) + 4;
    HChar emptyfifo_path[filename_len];
    VG_(snprintf)(emptyfifo_path, filename_len, "%s/%s-0", SGL_(clo).ipc_dir, PRISM_IPC_EMPTYFIFO_BASENAME);

    filename_len = ipc_dir_len + VG_(strlen)(PRISM_IPC_FULLFIFO_BASENAME) + 4;
    HChar fullfifo_path[filename_len];
    VG_(snprintf)(fullfifo_path, filename_len, "%s/%s-0", SGL_(clo).ipc_dir, PRISM_IPC_FULLFIFO_BASENAME);

    emptyfd = open_fifo(emptyfifo_path, VKI_O_RDONLY);
    fullfd  = open_fifo(fullfifo_path, VKI_O_WRONLY);
    shmem   = open_shmem(shmem_path, VKI_O_RDWR);

    /* initialize cached IPC state */
    ipc_ev_buf_idx = 0;
    set_and_init_buffer(ipc_ev_buf_idx);
    for (UInt i=0; i<PRISM_IPC_BUFFERS; ++i)
        ipc_buf_is_full[i] = False;

    initialized = True;
}


void SGL_(term_IPC)(void)
{
    tl_assert(initialized == True);

    /* send finish sequence */
    unsigned char* buf = curr_ev_buf_baseptr + curr_ev_buf_offset;
    SET_EV_END(buf);
    UInt finished = PRISM_IPC_FINISHED;
    if (VG_(write)(fullfd, &ipc_ev_buf_idx, sizeof(ipc_ev_buf_idx)) != sizeof(ipc_ev_buf_idx) ||
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
