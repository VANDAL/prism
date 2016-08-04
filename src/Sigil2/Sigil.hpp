#ifndef SIGIL_H
#define SIGIL_H

#include <map>
#include <chrono>

#include "Primitive.h"
#include "Backends.hpp"
#include "EventManager.hpp"

class Sigil
{
  public:
    std::string instance_id;

    using ToolName = std::string;
    using Args = std::vector<std::string>;
    using BackendArgparser = std::function<void(Args)>;
    using BackendFactory = std::function<std::shared_ptr<Backend>(void)>;
    using Parser = std::function<void(Args)>;
    using Exit = std::function<void(void)>;

    /* Frontend gets:
     * - the executable and its args
     * - args specifically for the frontend
     * - number of threads in the system 
     * - a unique timestamp to help uniquify files */
    using FrontendStarter = std::function<void(Args, Args, uint16_t, const std::string&)>;

    static Sigil &instance()
    {
        static Sigil singleton;
        return singleton;
    }

    /* main interface */
    void parseOptions(int argc, char *argv[]);
    void generateEvents();
    void registerFrontend(std::string name, FrontendStarter frontend_start);

    /* Event generation interface
     * If Sigil2 is configured to run with N>1 threads,
     * then the frontend should be run with N threads.
     * When the frontend generates events, it must identify
     * which thread generated the event by setting 'idx' (0-based) */
    template <typename T>
    void addEvent(const T &ev, uint16_t idx = 0)
    {
        assert(mgr != nullptr);
        mgr->addEvent(ev, idx);
    }

    /* static plugin interface */
    void registerBackend(ToolName name, BackendFactory factory);;
    void registerParser(ToolName name, Parser parser);;
    void registerExit(ToolName name, Exit exit_routine);;

  private:
    Sigil() : instance_id(std::to_string(std::chrono::system_clock::now().time_since_epoch().count())) {};

    int num_threads;

    std::shared_ptr<sgl::EventManager> mgr;
    std::map<ToolName, std::tuple<BackendFactory, Parser, Exit>> backend_registry;
    std::map<std::string, FrontendStarter> frontend_registry;

    BackendFactory create_backend;
    Exit exit_backend;
    std::function<void(void)> parse_backend;

    std::function<void(void)> start_frontend;
};

#endif
