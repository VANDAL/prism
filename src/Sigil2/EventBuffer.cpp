#include "SigiLog.hpp"
#include "EventBuffer.hpp"

namespace sgl
{

int EventBuffer::g_id = 0;

EventBuffer::EventBuffer()
    : full(0)
    , empty(MAX_BUFFERS)
    , prod_idx(MAX_BUFFERS)
    , cons_idx(MAX_BUFFERS)
{
    m_id = ++g_id;
    /* initialize producer-consumer state */
    empty.P();
    prod_buf = &bufbuf[prod_idx.increment()];
}


EventBuffer::~EventBuffer()
{
    size_t event_buffer_size = MAX_BUFFERS*sizeof(Buffer);
    SigiLog::debug("Backend Event Buffer size: " + std::to_string(event_buffer_size));
}


void EventBuffer::flush(Backend &backend)
{
    assert(prod_buf != nullptr);
    full.P();

    Buffer &buf = bufbuf[cons_idx.increment()];

    for (decltype(buf.used) i = 0; i < buf.used; ++i)
    {
        BufferedSglEv &ev = buf.events[i];

        switch (ev.tag)
        {
        case EvTag::SGL_MEM_TAG:
            backend.onMemEv(ev.mem);
            break;

        case EvTag::SGL_COMP_TAG:
            backend.onCompEv(ev.comp);
            break;

        case EvTag::SGL_SYNC_TAG:
            backend.onSyncEv(ev.sync);
            break;

        case EvTag::SGL_CXT_TAG:
            backend.onCxtEv(ev.cxt);
            break;

        default:
            /* control flow unimplemented */
            SigiLog::fatal(std::string("Received unhandled event in ").append(__FILE__));
        }
    }

    buf.used = 0;

    empty.V();
}


void EventBuffer::complete()
{
    full.V();
}


bool EventBuffer::isEmpty()
{
    return (empty.count >= MAX_BUFFERS);
}

}; //end namespace sgl
