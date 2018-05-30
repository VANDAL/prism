#!/bin/python

import sys
import os
from warnings import warn
import capnp
import STEventTraceUncompressed_capnp


def processSTEventTrace(file):
    unique_reads = set()
    unique_writes = set()
    total_comp = 0
    total_messages = 0
    for stream in (STEventTraceUncompressed_capnp.EventStreamUncompressed
                   .read_multiple_packed(file, traversal_limit_in_words=2**63)):
        total_messages += 1
        for event in stream.events:
            which = event.which()
            if which == 'comp':
                total_comp += 1
                if event.comp.mem == 'write':
                    # address range written
                    unique_writes.add(event.comp.startAddr)
                elif event.comp.mem == 'read':
                    # address range read
                    unique_reads.add(event.comp.startAddr)

    print("Total Messages: {}".format(total_messages))
    print("Total Comp: {}".format(total_comp))
    print("Unique Reads: {}".format(len(unique_reads)))
    print("Unique Writes: {}".format(len(unique_writes)))


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
