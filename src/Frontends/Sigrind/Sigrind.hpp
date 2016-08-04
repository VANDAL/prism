#ifndef SGL_SIGRIND_H
#define SGL_SIGRIND_H

#include <string>
#include <vector>
#include "SigrindIPC.h"

/* Sigil2 receives events from Valgrind via a set of shared memory buffers
 * There should exist at least 2 buffers, so Valgrind can fill a buffer
 * with events while Sigil2 reads from the other buffer. The optimal amount
 * of buffering done depends on the system resources and the variation in
 * event processing in the backend.
 *
 * A set of named pipes is used for syncing buffering between Sigil2 and
 * Valgrind. This allows each process to block when they cannot proceed,
 * compared to other IPC synchronization methods such as spinning.
 *
 * Because Valgrind serializes multithreaded applications (only allowing
 * one thread to run at a time), Sigil2 treats Valgrind as single-threaded.
 * Only one set of named pipes and buffers is necessary, for the one Valgrind
 * thread.
 * The Valgrind tool, Sigrind, will send a special event marker (thread swap)
 * to let Sigil2 know that the event stream switched to a new thread.
 */

namespace sgl
{

class Sigrind
{
    const std::string timestamp;
    const std::string shmem_file;
    const std::string empty_file;
    const std::string full_file;

    bool finished = false;

    /* fifos */
    int emptyfd;
    int fullfd;
    int full_data;

    /* multithreaded backend */
    const int num_threads;
    int be_idx;

    /* shared mem */
    SigrindSharedData *shared_mem;

  public:
    Sigrind(int num_threads, const std::string &tmp_dir, const std::string &instance_id);
    ~Sigrind();
    void produceSigrindEvents();

    /* The start routine run by Sigil2 to begin event generation.
     * Each frontend requires this function signature.
     *
     * Valgrind will be forked off as a separate process and begin
     * generating events to send to Sigil2.
     */
    static void start(const std::vector<std::string> &user_exec,
                      const std::vector<std::string> &args,
                      const uint16_t num_threads,
                      const std::string &instance_id);

  private:
    /* Clean up actions if an unexpected quit happens */
    void setInterruptOrTermHandler();

    /* Initialize IPC between Sigil2 and Valgrind */

    void initShMem();
    void makeNewFifo(const char *path) const;
    void connectValgrind();

    /* Read an int from 'full' fifo and return the data
     *
     * This is an index to the buffer array in the shared memory.
     * It informs Sigil2 that buffer[idx] has been filled with
     * events and can be passed to the Sigil2 event manager/buffers.
     */
    int readFullFifo();


    /* Write 'idx' to 'empty' fifo.
     *
     * This is the index to the buffer array in the shared memory.
     * It informs valgrind that buffer[idx] has been emptied by
     * Sigil2. Valgrind can then take ownership of that buffer and fill
     * it with events
     */
    void writeEmptyFifo(unsigned int idx);

    /* Implicitly take ownership of buffer[idx] in the shared memory,
     * and read events from (0 to used-1).
     * Those events are sequentially passed up the chain to the backend.
     */
    void produceFromBuffer(unsigned int idx, unsigned int used);
};

};

#endif
