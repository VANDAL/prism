#ifndef SIGIL_BACKEND_H
#define SIGIL_BACKEND_H

#include "Primitive.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>

/* Interface for Sigil2 backends.
 *
 * Each backend provides a handler for
 * each Sigil2 event type, and what to do on
 * exit */
class BackendIface
{
  public:
    virtual void onMemEv (const SglMemEv &)  {}
    virtual void onCompEv(const SglCompEv &) {}
    virtual void onSyncEv(const SglSyncEv &) {}
    virtual void onCxtEv (const SglCxtEv &)  {}
    virtual void onCFEv  (const SglCFEv &)   {}
};

using ToolName = std::string;
using Args = std::vector<std::string>;
using BackendPtr = std::shared_ptr<BackendIface>;
using BackendGenerator = std::function<BackendPtr(void)>;

/* Args passed from the command line to the backend */
using BackendParser = std::function<void(const Args &)>;

/* Invoked one time once all events have been passed to the backend */
using BackendFinish = std::function<void(void)>;

struct Backend
{
    BackendGenerator generator;
    BackendParser parser;
    BackendFinish finish;
    Args args;
};


class BackendFactory
{
  public:
    BackendFactory()  = default;
    ~BackendFactory() = default;

    auto create(ToolName name, Args args)         const -> Backend;
    auto add(ToolName name, BackendGenerator generator) -> void;
    auto add(ToolName name, BackendParser parser)       -> void;
    auto add(ToolName name, BackendFinish finish)       -> void;
    auto exists(ToolName name)                    const -> bool;
    auto available()                              const -> std::vector<std::string>;

  private:
    std::map<ToolName, Backend> registry;
};

#endif
