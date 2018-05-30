@0xb9c6524d44427f77;

struct EventStreamUncompressed {

  struct Event {

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

    enum MemType {
      none  @0;
      read  @1;
      write @2;
    }

    # A SynchroTrace event
    union {
      comp :group {
        # computation event
        # aggregate of one or more of type:
        # - integer operation
        # - floating point operation
        # with one:
        # - memory operation

        iops      @0 :UInt16;
        flops     @1 :UInt16;
        mem       @2 :MemType;
        startAddr @3 :UInt64;
        endAddr   @4 :UInt64;
      }

      comm :group {
        # communication event
        # reads that were written by another thread
        # i.e. userspace 'communication' between threads

        producerThread @5 :UInt16;
        producerEvent  @6 :UInt32;
        # the event number from the thread that wrote the address
        # event numbers start from 0 and increment until the thread completes

        startAddr @7 :UInt64;
        endAddr   @8 :UInt64;
      }

      sync :group {
        # synchronization event
        type @9  :SyncType;
        args @10 :List(UInt64);
      }

      marker :group {
        # instruction marker
        count @11 :UInt16;
      }
    }
  }

  events @0 :List(Event);
}
