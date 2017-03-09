@0x9274197a8c1bd9a8;

struct Event {

  struct AddrRange {
    start @0 :UInt64;
    end   @1 :UInt64;
  }

  struct CommEdge {
    producerThread @0 :UInt16;
    producerEvent  @1 :UInt32;
    # the event number from the thread that wrote the address
    # event numbers start from 0 and increment until the thread completes

    addrs @2 :List(AddrRange);
  }

  enum SyncType {
    spawn         @0;
    join          @1;
    barrier       @2;
    sync          @3;
    lock          @4;
    unlock        @5;
    condWait      @6;
    condSignal    @7;
    condBroadcast @8;
    spinLock      @9;
    spinUnlock    @10;
  }

  # A SynchroTrace event
  union {
    comp :group {
      # computation event
      # aggregate of one or more of type:
      # - integer operation
      # - floating point operation
      # - memory operation

      iops       @0 :UInt16;
      flops      @1 :UInt16;
      writes     @2 :UInt16;
      reads      @3 :UInt16; # reads from addresses written by this thread
      writeAddrs @4 :List(AddrRange);
      readAddrs  @5 :List(AddrRange);
    }

    comm :group {
      # communication event
      # reads from addresses that were written by other threads
      # i.e. userspace 'communication' between threads

      edges @6 :List(CommEdge);
    }

    sync :group {
      # synchronization event
      type @7 :SyncType;
      id   @8 :UInt64;
    }

    marker :group {
      # instruction marker
      count @9 :UInt16;
    }
  }
}

struct EventStream {
  events @0 :List(Event);
}
