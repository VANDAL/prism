#ifndef SIGIL_BACKEND_H
#define SIGIL_BACKEND_H

#include "Primitive.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>

class BackendIface
{
  public:
    virtual ~BackendIface() {}
    virtual void onMemEv(const sigil2::MemEvent &) {}
    virtual void onCompEv(const sigil2::CompEvent &) {}
    virtual void onSyncEv(const sigil2::SyncEvent &) {}
    virtual void onCxtEv(const sigil2::CxtEvent &) {}
    virtual void onCFEv(const SglCFEv &) {}
};

using ToolName = std::string;
using Args = std::vector<std::string>;
using BackendPtr = std::unique_ptr<BackendIface>;
using BackendIfaceGenerator = std::function<BackendPtr(void)>;

using BackendParser = std::function<void(const Args &)>;
/* Args passed from the command line to the backend */

using BackendFinish = std::function<void(void)>;
/* Invoked one time once all events have been passed to the backend */

struct Backend
{
    BackendIfaceGenerator generator;
    BackendParser parser;
    BackendFinish finish;
    sigil2::capabilities caps;
    Args args;
};


class BackendFactory
{
  public:
    BackendFactory()  = default;
    ~BackendFactory() = default;

    auto create(ToolName name, Args args) const -> Backend;
    auto add(ToolName name, Backend be) -> void;
    auto exists(ToolName name) const -> bool;
    auto available() const -> std::vector<std::string>;

  private:
    std::map<ToolName, Backend> registry;
};

#endif
