//===- PrismLLVMAppIface.cpp ----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the interface of Prism instrumented applications
// to the the Prism workload analysis framework. Applications are
// instrumented to invoke these subroutines.
//
//===----------------------------------------------------------------------===//
#include "PrismLLVMAppIface.hpp"
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

namespace prism {
Config::Config() {
  for (auto var : envvars) {
    char *val = getenv(var);
    if (val == nullptr) // require that all variables must be set
      fatal("env variable missing: {}", var);
    env[var] = val;
  }
}

const Config::value_type& Config::operator[](const key_type& key) {
  if (env.find(key) == env.end())
    fatal("looked up missing config key: {}", key);
  return env[key];
}

namespace ipc {
FdRAII::FdRAII(int fd) : fd(fd) {}

FdRAII::FdRAII(const std::string &filepath, int openflags)
    : fd((exitOnTimeoutNonexistent(filepath),
          openAndCheckResult(filepath, openflags))) {}

FdRAII::FdRAII(FdRAII &&other) {
  std::swap(fd, other.fd);
}

FdRAII& FdRAII::operator=(FdRAII &&other) {
  std::swap(fd, other.fd);
  return *this;
}

FdRAII::~FdRAII() {
  if (fd > -1) {
    if (close(fd) < 0) {
      fmt::printf("channel did not close properly");
    }
  }
}

void FdRAII::exitOnTimeoutNonexistent(const std::string &filepath) {
  const unsigned waits = 10;
  unsigned count;
  timespec ts;

  count = waits;
  ts.tv_sec = 0;
  ts.tv_nsec = 200000000L;

  for (; count > 0; --count) {
    if (access(filepath.c_str(), F_OK))
      break;
    nanosleep(&ts, nullptr);
  }

  if (count == 0)
    fatal("Could not connect to Prism IPC: {}", filepath);
}

int FdRAII::openAndCheckResult(const std::string &filepath, int openflags) {
  int fd = open(filepath.c_str(), openflags);
  if (fd == -1)
    fatal("{}: opening Prism IPC file {}", strerror(errno), filepath);
  return fd;
}

Channel::Channel(const std::string& tmpdir, int idx)
    : shmemFd(tmpdir + "/" + PRISM_IPC_SHMEM_BASENAME + "-" + std::to_string(idx),
              O_RDWR)
    , emptyFifoFd(tmpdir + "/" + PRISM_IPC_EMPTYFIFO_BASENAME + "-" + std::to_string(idx),
                  O_RDONLY)
    , fullFifoFd(tmpdir + "/" + PRISM_IPC_FULLFIFO_BASENAME + "-" + std::to_string(idx),
                 O_WRONLY) {}

void Channel::lock() {
}

Connection::Connection(Channel &channel)
    : channel(channel) {
}

EventBuffer *Connection::acquireBuffer() {
  TODO
  // acquire exclusive access to Channel
  // get the event buffer from Channel
}

void Connection::releaseBuffer() {
  TODO
  // tell Connection we are done with the event buffer
  //    Connection will flush the buffer
  // release exclusive access to Channel, allowing another thread
  // (or this one again) to acquire it
}

namespace {
int stoiWrapper(const std::string &str, int base=10) {
  try {
    return stoi(str, nullptr, base);
  } catch(std::exception &e) {
    fatal("string-to-int conversion exception: {}", str);
  }
}
} // end namespace.

ConnectionWrapper::ConnectionWrapper() {
  std::lock_guard<std::mutex> lock(channelsMtx);
  static int numChannels = stoiWrapper(config[ENV_PRISM_NUM_CHANNELS]);
  static std::string tmpdir = config[ENV_PRISM_TMPDIR];
  id = channels.size();
  unsigned idx = id % numChannels;
  channels.emplace_back(tmpdir, idx);

  // set member variables
}

void ConnectionWrapper::ensureSpaceForX(size_t x) {
  if ((local_used + x) >= PRISM_EVENTS_BUFFER_SIZE) {
    //update connection
  }
}
} // end namespace ipc.

// for easier lookup in LLVM instrumentation
extern "C" {
/// Initialize new channel for each new thread.
/// Let clang manage TLS storage models, instead of manually managing in
/// LLVM instrumentation.
// Non-local variables with thread-local storage duration are initialized
// as part of thread launch, sequenced-before the execution of the thread
// function.
thread_local ipc::ConnectionWrapper connection;
void prism_ensure_enough_space_for_x(size_t x) { connection.ensureSpaceForX(x); }
PrismEvVariant *prism_cur_ev_get() { return connection.cur_ev; }
PrismEvVariant *prism_end_ev_get() { return connection.end_ev; }
} // end extern "C".
} // end namespace prism.
