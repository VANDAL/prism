#ifndef SGL_DRSIGIL_H
#define SGL_DRSIGIL_H

#include "Sigil2/Frontends.hpp"
#include "DrSigilIPC.h"

#include <vector>
#include <string>
#include <mutex>
#include <thread>

namespace sgl
{

class DrSigil : public FrontendIface
{
    using FrontendIface::uid;
  public:
    DrSigil(const std::string &ipcDir);
    ~DrSigil();
    virtual auto acquireBuffer() -> EventBuffer* override;
    virtual auto releaseBuffer() -> void override;

  private:
    auto initShMem()                         -> void;
    auto makeNewFifo(const char *path) const -> void;
    auto readFullFifo()                const -> int;
    auto writeEmptyFifo(unsigned idx)  const -> void;
    auto connectDynamoRIO()                  -> void;
    auto disconnectDynamoRIO()               -> void;

    /* main event loop */
    auto receiveDynamoRIOEventsLoop() -> void;
    std::shared_ptr<std::thread> eventLoop;

    /* IPC (meta)data */
    const std::string shmemFile;
    const std::string emptyFile;
    const std::string fullFile;
    int emptyfd;
    int fullfd;
    DrSigilSharedData *sharedMem;

    /* Current frontend thread id;
     * A different thread id in the event stream triggers a 'thread switch'
     * event to be generated for the backend */
    int currThreadID{-1};

    /* Thread context swaps are handled in this
     * frontend manager, not in DynamoRIO */
    SglSyncEv threadSwapEvent;
};

}; //end namespace sgl

#endif
