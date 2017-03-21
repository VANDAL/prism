#!/bin/python

import sys
import os
from warnings import warn
import capnp
import STEventTraceCompressed_capnp


def processSTEventTrace(file):
    for stream in (STEventTraceCompressed_capnp.EventStreamCompressed
                   .read_multiple_packed(file, traversal_limit_in_words=2**63)):
        for event in stream.events:
            which = event.which()
            if which == 'comp':
                event.comp.iops    # IOPs value
                event.comp.flops   # FLOPs value
                event.comp.writes  # writes value
                event.comp.reads   # reads value
                for write in event.comp.writeAddrs:
                    write.start  # start of address range
                    write.end    # end of address range
                for read in event.comp.writeAddrs:
                    read.start  # start of address range
                    read.end    # end of address range
            elif which == 'comm':
                for edge in event.comm.edges:
                    # the thread-event tuple that generated
                    # this communication edge
                    edge.producerThread
                    edge.producerEvent
                    for addr in edge.addrs:
                        addr.start  # start of address range
                        addr.end    # end of address range
            elif which == 'sync':
                if event.sync.type == 'spawn':
                    event.sync.id  # spawned thread id
                elif event.sync.type == 'join':
                    event.sync.id  # joined thread id
                elif event.sync.type == 'barrier':
                    event.sync.id  # barrier id
                elif event.sync.type == 'sync':
                    event.sync.id
                elif event.sync.type == 'lock':
                    event.sync.id  # lock mutex
                elif event.sync.type == 'unlock':
                    event.sync.id  # unlock mutex
                elif event.sync.type == 'condWait':
                    event.sync.id  # condition variable
                elif event.sync.type == 'condSignal':
                    event.sync.id  # condition variable
                elif event.sync.type == 'condBroadcast':
                    event.sync.id  # condition variable
                elif event.sync.type == 'spinLock':
                    event.sync.id  # lock id
                elif event.sync.type == 'spinUnlock':
                    event.sync.id  # unlock id
                else:
                    raise Exception('unhandled sync event')
            elif which == 'marker':
                # the number of instructions since the last marker
                event.marker.count

if __name__ == '__main__':
    filepath = sys.argv[1]
    name, ext = os.path.splitext(filepath)
    if ext == '.gz':
        # https://github.com/jparyani/pycapnp/issues/80
        f = os.popen('cat ' + filepath + ' | gzip -d')
    else:
        if ext != '.bin':
            warn('not a .bin file')
        f = open(filepath, 'r')
    processSTEventTrace(f)
