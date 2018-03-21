#ifndef PRISM_BACKEND_H
#define PRISM_BACKEND_H

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
    virtual auto onMemEv(const prism::MemEvent &) -> void {}
    virtual auto onCompEv(const prism::CompEvent &) -> void {}
    virtual auto onSyncEv(const prism::SyncEvent &) -> void {}
    virtual auto onCxtEv(const prism::CxtEvent &) -> void {}
    virtual auto onCFEv(const PrismCFEv &) -> void {}
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
    prism::capabilities caps;
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
