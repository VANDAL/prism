#include "DrSigil.hpp"
#include "Sigil2/SigiLog.hpp"

#include <fcntl.h>
#include <sys/mman.h>

namespace sgl
{
using SigiLog::fatal;

DrSigil::DrSigil(const std::string &ipcDir)
    : shmemFile(ipcDir + "/" + DRSIGIL_SHMEM_NAME     + "-" + std::to_string(uid))
    , emptyFile(ipcDir + "/" + DRSIGIL_EMPTYFIFO_NAME + "-" + std::to_string(uid))
    , fullFile (ipcDir + "/" + DRSIGIL_FULLFIFO_NAME  + "-" + std::to_string(uid))
{
    /* initialize IPC */
    initShMem();
    makeNewFifo(emptyFile.c_str());
    makeNewFifo(fullFile.c_str());

    connectDynamoRIO();

    /* Begin managing DynamoRIO event buffers */
    eventLoop = std::make_shared<std::thread>(&DrSigil::receiveDynamoRIOEventsLoop, this);
}


DrSigil::~DrSigil()
{
    eventLoop->join();
    disconnectDynamoRIO();
}


auto DrSigil::acquireBuffer() -> EventBuffer*
{
    return nullptr;
}


auto DrSigil::releaseBuffer() -> void
{
}


auto DrSigil::initShMem() -> void
{
    FILE *fd = fopen(shmemFile.c_str(), "wb+");
    if (fd == nullptr)
        fatal(std::string("sigrind shared memory file open failed -- ") + strerror(errno));

    std::unique_ptr<DrSigilSharedData> init(new DrSigilSharedData());
    int count = fwrite(init.get(), sizeof(DrSigilSharedData), 1, fd);
    if (count != 1)
    {
        fclose(fd);
        fatal(std::string("sigrind shared memory file write failed -- ") + strerror(errno));
    }

    sharedMem = reinterpret_cast<DrSigilSharedData *>
                 (mmap(nullptr, sizeof(DrSigilSharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fd), 0));
    if (sharedMem == MAP_FAILED)
    {
        fclose(fd);
        fatal(std::string("sigrind mmap shared memory failed -- ") + strerror(errno));
    }

    fclose(fd);
}


auto DrSigil::makeNewFifo(const char *path) const -> void
{
    if (mkfifo(path, 0600) < 0)
    {
        if (errno == EEXIST)
        {
            if (remove(path) != 0)
                fatal(std::string("sigil2 could not delete old fifos -- ") + strerror(errno));

            if (mkfifo(path, 0600) < 0)
                fatal(std::string("sigil2 failed to create dynamorio fifos -- ") + strerror(errno));
        }
        else
        {
            fatal(std::string("sigil2 failed to create dynamorio fifos -- ") + strerror(errno));
        }
    }
}


auto DrSigil::readFullFifo() const -> int
{
    int val;
    int res = read(fullfd, &val, sizeof(val));
    if (res == 0)
        fatal("Unexpected end of fifo");
    else if (res < 0)
        fatal(std::string("could not read from dynamorio full fifo -- ") + strerror(errno));
    return val;
}


auto DrSigil::writeEmptyFifo(unsigned idx) const -> void
{
    if (write(emptyfd, &idx, sizeof(idx)) < 0)
        fatal(std::string("could not send dynamorio empty buffer status -- ") + strerror(errno));
}


auto DrSigil::connectDynamoRIO() -> void
{
    /* XXX Sigil2 might get stuck blocking if DynamoRIO
     * unexpectedly exits before connecting at this point */

    emptyfd = open(emptyFile.c_str(), O_WRONLY);
    if (emptyfd < 0)
        fatal(std::string("sigil2 failed to open dynamorio fifo for writing-- ") + strerror(errno));

    fullfd = open(fullFile.c_str(), O_RDONLY);
    if (fullfd < 0)
        fatal(std::string("sigil2 failed to open dynamorio fifo for reading -- ") + strerror(errno));
}


auto DrSigil::disconnectDynamoRIO() -> void
{
    munmap(sharedMem, sizeof(DrSigilSharedData));
    close(emptyfd);
    close(fullfd);
}


auto DrSigil::receiveDynamoRIOEventsLoop() -> void
{
    bool finished{false};
    while (finished == false)
    {
        unsigned fromDR = readFullFifo();
        unsigned idx;

        if (fromDR == DRSIGIL_FINISHED)
        {
            /* Partial buffer possible */
            finished = true;
            idx = readFullFifo();
        }
        else
        {
            idx = fromDR;
        }

        /* SOMETHING HAPPENS HERE */

        /* tell DynamoRIO that the buffer is empty again */
        writeEmptyFifo(idx);
    }
}

}; //end namespace sgl
