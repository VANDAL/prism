#ifndef SGL_DRSIGIL_H
#define SGL_DRSIGIL_H

#include <vector>
#include <string>
#include "DrSigilIPC.h"

/* Sigil2 receives events from DynamoRIO via a set of shared memory buffers
 *
 * Because DynamoRIO is multithreaded,
 *
 */

namespace sgl
{

class DrSigil
{
    /* The 'backend' thread currently receiving events.
     * Each DrSigil instance is tied to a backend thread */
    const int ipc_idx;

    /* Current frontend thread id;
     * A different thread id in the event stream triggers a 'thread switch'
     * event to be generated for the backend */
    int curr_thread_id;

    /* Set when the event stream end is detected */
    bool finished = false;

    /* Thread context swaps are handled in this
     * frontend manager, not in DynamoRIO */
    SglSyncEv thread_swap_event;

    const std::string shmem_file;
    const std::string empty_file;
    const std::string full_file;

    /* fifos */
    int emptyfd;
    int fullfd;
    int full_data;

    /* shared mem */
    DrSigilSharedData *shared_mem;

  public:
    DrSigil(int ipc_idx, std::string tmp_dir);
    ~DrSigil();
    void produceDynamoRIOEvents();

    /* The start routine run by Sigil2 to begin event generation.
     * Each frontend requires this function signature.
     */

    static int num_threads;
    static void start(const std::vector<std::string> &user_exec,
                      const std::vector<std::string> &args,
                      const uint16_t num_threads);

  private:
    void initShMem();
    void makeNewFifo(const char *path) const;
    void connectDynamoRIO();
    void disconnectDynamoRIO();

    int readFullFifo();
    void writeEmptyFifo(unsigned int idx);
    void produceFromBuffer(unsigned int idx, unsigned int used);
};

};

#endif

