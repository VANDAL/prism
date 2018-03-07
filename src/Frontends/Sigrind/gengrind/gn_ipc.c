#include "gn_ipc.h"
#include "gn_clo.h"
#include "coregrind/pub_core_libcfile.h"
#include "coregrind/pub_core_aspacemgr.h"
#include "coregrind/pub_core_syscall.h"
#include "pub_tool_basics.h"
#include "pub_tool_vki.h"       // errnum, vki_timespec
#include "pub_tool_vkiscnums.h" // __NR_nanosleep

static Bool initialized = False;
static Int gnEmptyFd;
static Int gnFullFd;
static Sigil2DBISharedData* gnShmem;

SglEvVariant *GN_(currEv);
SglEvVariant *GN_(endEv);
size_t *GN_(usedEv);
/* IPC channel */

static UInt          gnNextIdx;
static UInt          gnCurrIdx;
static EventBuffer   *gnCurrEvBuf;
//static SglEvVariant  *currEvSlot;
//static NameBuffer    *currNameBuf;
//static char*         currNameSlot;
/* cached IPC state */


static Bool isFull[SIGIL2_IPC_BUFFERS];
/* track available buffers */


//static inline void set_next_buffer(void)
//{
//    /* try the next buffer, circular */
//    ++curr_idx;
//    if (curr_idx == SIGIL2_IPC_BUFFERS)
//        curr_idx = 0;
//
//    /* if the next buffer is full,
//     * wait until Sigil2 communicates that it's free */
//    if (is_full[curr_idx])
//    {
//        UInt buf_idx;
//        Int res = VG_(read)(gnEmptyFd, &buf_idx, sizeof(buf_idx));
//        if (res != sizeof(buf_idx))
//        {
//            VG_(umsg)("error VG_(read)\n");
//            VG_(umsg)("error reading from Sigrind fifo\n");
//            VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
//            VG_(exit)(1);
//        }
//
//        tl_assert(buf_idx < SIGIL2_IPC_BUFFERS);
//        tl_assert(buf_idx == curr_idx);
//        curr_idx = buf_idx;
//        is_full[curr_idx] = False;
//    }
//
//    set_and_init_buffer(curr_idx);
//}
//
//
//static inline Bool is_events_full(void)
//{
//    return curr_ev_buf->used == SIGIL2_EVENTS_BUFFER_SIZE;
//}
//
//
//static inline Bool is_names_full(UInt size)
//{
//    return (curr_name_buf->used + size) > SIGIL2_EVENTS_BUFFER_SIZE;
//}
//
//
//SglEvVariant* GN_(acq_event_slot)()
//{
//    tl_assert(initialized == True);
//
//    if (is_events_full())
//    {
//        flush_to_sigil2();
//        set_next_buffer();
//    }
//
//    curr_ev_buf->used++;
//    return curr_ev_slot++;
//}
//
//
//EventNameSlotTuple GN_(acq_event_name_slot)(UInt size)
//{
//    tl_assert(initialized == True);
//
//    if (is_events_full() || is_names_full(size))
//    {
//        flush_to_sigil2();
//        set_next_buffer();
//    }
//
//    EventNameSlotTuple tuple = {curr_ev_slot, curr_name_slot, curr_name_buf->used};
//    curr_ev_buf->used   += 1;
//    curr_ev_slot        += 1;
//    curr_name_buf->used += size;
//    curr_name_slot      += size;
//
//    return tuple;
//}


//-------------------------------------------------------------------------------------------------
/** Initialization/Termination **/

static int openFifo(const HChar *fifo_path, int flags)
{
    tl_assert(initialized == False);

    int tries = 0;
    const int max_tries = 4;
    int fd = VG_(fd_open)(fifo_path, flags, 0600);
    while (fd < 0) {
        if (++tries < max_tries) {
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
        else {
            VG_(umsg)("FIFO for Sigrind failed\n");
            VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
            VG_(exit) (1);
        }
    }

    return fd;
}


static Sigil2DBISharedData* openShmem(const HChar *gnShmem_path, int flags)
{
    tl_assert(initialized == False);

    int shared_mem_fd = VG_(fd_open)(gnShmem_path, flags, 0600);
    if (shared_mem_fd < 0) {
        VG_(umsg)("Cannot open shared_mem file %s\n", gnShmem_path);
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }

    SysRes res = VG_(am_shared_mmap_file_float_valgrind)(sizeof(Sigil2DBISharedData),
                                                         VKI_PROT_READ|VKI_PROT_WRITE,
                                                         shared_mem_fd, (Off64T)0);
    if (sr_isError(res)) {
        VG_(umsg)("error %lu %s\n", sr_Err(res), VG_(strerror)(sr_Err(res)));
        VG_(umsg)("error VG_(am_shared_mmap_file_float_valgrind) %s\n", gnShmem_path);
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }

    Addr addr_shared = sr_Res (res);
    VG_(close)(shared_mem_fd);

    return (Sigil2DBISharedData*) addr_shared;
}


void GN_(initIPC)()
{
    tl_assert(initialized == False);

    if (GN_(clo).standalone_test == True) {
        gnShmem = VG_(malloc)("gn.test.buffer", sizeof(*gnShmem));

        gnNextIdx = 0;
        gnCurrEvBuf = gnShmem->eventBuffers + gnNextIdx;
        gnCurrEvBuf->used = 0;
        GN_(currEv) = gnCurrEvBuf->events + gnCurrEvBuf->used;
        GN_(usedEv) = &gnCurrEvBuf->used;

        /* ensure events is an array, not a pointer */
        tl_assert(sizeof(gnCurrEvBuf->events) != sizeof(gnCurrEvBuf->events[0]));
        GN_(endEv) = gnCurrEvBuf->events + sizeof(gnCurrEvBuf->events)/sizeof(gnCurrEvBuf->events[0]);
        for (UInt i=0; i<SIGIL2_IPC_BUFFERS; ++i)
            isFull[i] = False;

        ++gnNextIdx;

        initialized = True;
        return;
    }

    if (GN_(clo).ipc_dir == NULL) {
        VG_(fmsg)("No --ipc-dir argument found, shutting down...\n");
        VG_(exit)(1);
    }

    Int ipcDirLen = VG_(strlen)(GN_(clo).ipc_dir);
    Int filenameLen;

    //len is strlen + null + other chars (/ and -0)
    filenameLen = ipcDirLen + VG_(strlen)(SIGIL2_IPC_SHMEM_BASENAME) + 4;
    HChar gnShmemPath[filenameLen];
    VG_(snprintf)(gnShmemPath, filenameLen, "%s/%s-0", GN_(clo).ipc_dir, SIGIL2_IPC_SHMEM_BASENAME);

    filenameLen = ipcDirLen + VG_(strlen)(SIGIL2_IPC_EMPTYFIFO_BASENAME) + 4;
    HChar emptyfifoPath[filenameLen];
    VG_(snprintf)(emptyfifoPath, filenameLen, "%s/%s-0", GN_(clo).ipc_dir, SIGIL2_IPC_EMPTYFIFO_BASENAME);

    filenameLen = ipcDirLen + VG_(strlen)(SIGIL2_IPC_FULLFIFO_BASENAME) + 4;
    HChar fullfifoPath[filenameLen];
    VG_(snprintf)(fullfifoPath, filenameLen, "%s/%s-0", GN_(clo).ipc_dir, SIGIL2_IPC_FULLFIFO_BASENAME);

    gnEmptyFd = openFifo(emptyfifoPath, VKI_O_RDONLY);
    gnFullFd  = openFifo(fullfifoPath, VKI_O_WRONLY);
    gnShmem   = openShmem(gnShmemPath, VKI_O_RDWR);

    /* initialize cached IPC state */
    GN_(currEv) = NULL;
    GN_(endEv) = NULL;
    gnCurrIdx = 0;
    gnNextIdx = 0;
    GN_(setNextBuffer)();
    for (UInt i=0; i<SIGIL2_IPC_BUFFERS; ++i)
        isFull[i] = False;

    initialized = True;
}


void GN_(termIPC)(void)
{
    tl_assert(initialized == True);

    if (GN_(clo).standalone_test == True) {
        VG_(free)(gnShmem);
        gnShmem = NULL;
        gnCurrEvBuf = NULL;
        GN_(currEv) = NULL;
        GN_(usedEv) = NULL;
        GN_(endEv) = NULL;
        return;
    }


    /* Flush the remaining buffer ... */
    GN_(flushCurrBuffer)();

    /* ... and send finish sequence */
    UInt finished = SIGIL2_IPC_FINISHED;
    if (VG_(write)(gnFullFd, &finished, sizeof(finished)) != sizeof(finished)) {
        VG_(umsg)("error VG_(write)\n");
        VG_(umsg)("error writing to Sigrind fifo\n");
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }

    /* wait until Sigrind disconnects */
    while (VG_(read)(gnEmptyFd, &finished, sizeof(finished)) > 0);

    VG_(close)(gnEmptyFd);
    VG_(close)(gnFullFd);
}


void GN_(flushCurrBuffer)(void)
{
    /* Mark that the buffer is being flushed,
     * and tell Sigil2 the buffer is ready to consume */
    isFull[gnCurrIdx] = True;
    Int res = VG_(write)(gnFullFd, &gnCurrIdx, sizeof(gnCurrIdx));
    if (res != sizeof(gnCurrIdx)) {
        VG_(umsg)("error VG_(write)\n");
        VG_(umsg)("error writing to Sigrind fifo\n");
        VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
        VG_(exit)(1);
    }
}


void GN_(setNextBuffer)(void)
{
    /* try the next buffer, circular */
    if (gnNextIdx == SIGIL2_IPC_BUFFERS)
        gnNextIdx = 0;

    /* if the next buffer is full,
     * wait until Sigil2 communicates that it's free */
    if (isFull[gnNextIdx]) {
        UInt bufIdx;
        Int res = VG_(read)(gnEmptyFd, &bufIdx, sizeof(bufIdx));
        if (res != sizeof(bufIdx)) {
            VG_(umsg)("error VG_(read)\n");
            VG_(umsg)("error reading from Sigrind fifo\n");
            VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
            VG_(exit)(1);
        }

        tl_assert(bufIdx < SIGIL2_IPC_BUFFERS);
        tl_assert(bufIdx == gnNextIdx);
        isFull[gnNextIdx] = False;
    }

    gnCurrEvBuf = gnShmem->eventBuffers + gnNextIdx;
    gnCurrEvBuf->used = 0;
    GN_(currEv) = gnCurrEvBuf->events + gnCurrEvBuf->used;
    GN_(usedEv) = &gnCurrEvBuf->used;

    /* ensure events is an array, not a pointer */
    tl_assert(sizeof(gnCurrEvBuf->events) != sizeof(gnCurrEvBuf->events[0]));
    GN_(endEv) = gnCurrEvBuf->events + sizeof(gnCurrEvBuf->events)/sizeof(gnCurrEvBuf->events[0]);

    //currNameBuf = gnShmem->nameBuffers + currIdx;
    //currNameBuf->used = 0;
    //currNameSlot = curr_name_buf->names + curr_name_buf->used;

    gnCurrIdx = gnNextIdx;
    ++gnNextIdx;
}


void GN_(flushCurrAndSetNextBuffer)(void)
{
    if (GN_(clo).standalone_test == True) {
        gnCurrEvBuf = gnShmem->eventBuffers + gnNextIdx;
        gnCurrEvBuf->used = 0;
        GN_(currEv) = gnCurrEvBuf->events + gnCurrEvBuf->used;
        GN_(usedEv) = &gnCurrEvBuf->used;

        /* ensure events is an array, not a pointer */
        tl_assert(sizeof(gnCurrEvBuf->events) != sizeof(gnCurrEvBuf->events[0]));
        GN_(endEv) = gnCurrEvBuf->events + sizeof(gnCurrEvBuf->events)/sizeof(gnCurrEvBuf->events[0]);

        ++gnNextIdx;
    }
    else {
        GN_(flushCurrBuffer)();
        GN_(setNextBuffer)();
    }
}
