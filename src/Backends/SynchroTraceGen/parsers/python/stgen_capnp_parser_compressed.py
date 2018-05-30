#!/bin/python

# Sample script to parse a capnproto trace output by SynchroTraceGen

import os
import argparse

# Import hook magic to load capnproto schemas
# Note that each schema (compressed/uncompressed
#   must be processed differently
import capnp
import STEventTraceCompressed_capnp


def process_comp(comp):
    comp.iops    # IOPs value
    comp.flops   # FLOPs value
    comp.writes  # writes value
    comp.reads   # reads value

    for write in comp.writeAddrs:
        write.start  # start of address range
        write.end    # end of address range

    for read in comp.readAddrs:
        read.start  # start of address range
        read.end    # end of address range


def process_comm(comm):
    for edge in comm.edges:
        # the thread-event tuple that generated
        #   this communication edge
        edge.producerThread
        edge.producerEvent

        for addr in edge.addrs:
            addr.start  # start of address range
            addr.end    # end of address range


def process_sync(sync):
    assert(len(sync.args) < 3)
    if sync.type == 'spawn':
        sync.args[0]  # spawned thread id
    elif sync.type == 'join':
        sync.args[0]  # joined thread id
    elif sync.type == 'barrier':
        sync.args[0]  # barrier id
    elif sync.type == 'sync':
        sync.args
    elif sync.type == 'lock':
        sync.args[0]  # lock mutex
    elif sync.type == 'unlock':
        sync.args[0]  # unlock mutex
    elif sync.type == 'condWait':
        sync.args[0]  # condition variable
    elif sync.type == 'condSignal':
        sync.args[0]  # condition variable
    elif sync.type == 'condBroadcast':
        sync.args[0]  # condition variable
    elif sync.type == 'spinLock':
        sync.args[0]  # lock id
    elif sync.type == 'spinUnlock':
        sync.args[0]  # unlock id
    else:
        raise Exception('unhandled sync event')


def process_marker(marker):
    marker.count  # the number of instructions since the last marker


def parse_stgen_trace_compressed(f):
    for message in (STEventTraceCompressed_capnp.EventStreamCompressed
                    .read_multiple_packed(f, traversal_limit_in_words=2**63)):
        for event in message.events:
            which = event.which()
            if which == 'comp':
                process_comp(event.comp)
            elif which == 'comm':
                process_comm(event.comm)
            elif which == 'sync':
                process_sync(event.sync)
            elif which == 'marker':
                process_marker(event.marker)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=('Sample script to parse '
                                                  'SynchroTraceGen traces'))
    parser.add_argument('tracepath')
    args = parser.parse_args()

    tracepath = args.tracepath
    name, ext = os.path.splitext(tracepath)

    # https://github.com/jparyani/pycapnp/issues/80
    f = os.popen('cat ' + tracepath + ' | gzip -d') if ext == '.gz' \
        else open(tracepath, 'r') if ext == '.bin' \
        else None

    if not f:
        raise('Unknown file extension')

    parse_stgen_trace_compressed(f)
