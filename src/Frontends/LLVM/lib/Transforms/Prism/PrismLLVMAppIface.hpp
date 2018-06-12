//===- PrismLLVMAppIface.hpp ----------------------------------------------===//
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
#ifndef PRISM_LLVM_APP_IFACE_H
#define PRISM_LLVM_APP_IFACE_H

#include <mutex>
#include <map>
#include <string>
#include "spdlog/fmt/fmt.h"
#include "Frontends/CommonShmemIPC.h"

namespace prism {
template<typename ...Args>
[[noreturn]] void fatal(const char *msg, Args ...args) {
	fmt::print(stderr, msg, args...);
	std::exit(EXIT_FAILURE);
}

constexpr const char* ENV_PRISM_TMPDIR = "PRISM_TMPDIR";
constexpr const char* ENV_PRISM_NUM_CHANNELS = "PRISM_NUM_CHANNELS";
constexpr const char* ENV_PRISM_ENABLE_INSTR = "PRISM_ENABLE_INSTR";
constexpr const char* envvars[] = {ENV_PRISM_TMPDIR,
                                   ENV_PRISM_NUM_CHANNELS,
                                   ENV_PRISM_ENABLE_INSTR};

/// Config data is passed in via environment.
class Config {
  using key_type = std::string;
  using value_type = std::string;
  std::map<key_type, value_type> env;
public:
  Config();
  const value_type& operator[](const key_type& key);
};

namespace ipc {
/// Automatically cleans up resources.
class FdRAII {
  int fd;

public:
  FdRAII(int fd);
  FdRAII(const std::string &filepath, int openflags);
  FdRAII(const FdRAII &other) = delete;
  FdRAII& operator=(const FdRAII &other) = delete;
  FdRAII(FdRAII &&other);
  FdRAII& operator=(FdRAII &&other);
  ~FdRAII();

private:
  void exitOnTimeoutNonexistent(const std::string &filepath);
  int openAndCheckResult(const std::string &filepath, int openflags);
};

/// Manages a single IPC channel to Prism.
// Channels are limited by the amount created in Prism.
class Channel {
  FdRAII shmemFd;
  FdRAII emptyFifoFd;
  FdRAII fullFifoFd;
  PrismDBISharedData *shmem;
  std::mutex mtx;

public:
  Channel(const std::string& tmpdir, int idx);
  void lock();
  void unlock();

  EventBuffer *getBuffer();

private:

  void finishCurrentBuffer();
  EventBuffer *incrCurrentBuffer();
};

/// Manages a single thread->channel connection
class Connection {
  size_t id;
  Channel &channel;
public:
  Connection(Channel& channel);
  EventBuffer *acquireBuffer();
  void releaseBuffer();
};

/// Manages all channels to Prism.
// Usage is to construct a new instance for each new 'thread' context,
// and then insert helper functions in LLVM instrumentation to acquire
// a buffer to send events to Prism
class ConnectionWrapper {
  static std::vector<ipc::Channel> channels;
  static std::mutex channelsMtx;
  static Config config;
  size_t id;
public:
  /// Each channel is set up during construction.
  ConnectionWrapper();
  void ensureSpaceForX(size_t x);

  /// Directly used in LLVM instrumentation
  size_t *used;
  size_t local_used;
  PrismEvVariant *cur_ev;
  PrismEvVariant *end_ev; // expected to be one-past-the-end of event array
};
} // end namespace ipc.

extern "C" {
void prism_ensure_enough_space_for_x(size_t x);
PrismEvVariant *prism_cur_ev_get();
PrismEvVariant *prism_end_ev_get();
} // end extern "C".
} // end namespace prism.

#endif
