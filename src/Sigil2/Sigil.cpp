#include <algorithm>

#include "SigiLog.hpp"

#include "InstrumentationIface.h"
#include "SigilParser.hpp"
#include "Sigil.hpp"


////////////////////////////////////////////////////////////
// event generation loop
////////////////////////////////////////////////////////////
void Sigil::generateEvents()
{
    assert(start_frontend != nullptr);

    assert(parse_backend != nullptr);
    assert(create_backend != nullptr);
    assert(exit_backend != nullptr);

    assert(num_threads > 0);

    /* the event consumers (the backend)
     * are started in the event manager */
    parse_backend();
    mgr = std::make_shared<sgl::EventManager>(num_threads, create_backend);

    start_frontend();
    mgr->finish();

    exit_backend();
}


////////////////////////////////////////////////////////////
// instrumentation interface
////////////////////////////////////////////////////////////
void SGLnotifyMem(SglMemEv ev)
{
    Sigil::instance().addEvent(ev);
}


void SGLnotifyComp(SglCompEv ev)
{
    Sigil::instance().addEvent(ev);
}


void SGLnotifyCxt(SglCxtEv ev)
{
    Sigil::instance().addEvent(ev);
}


void SGLnotifySync(SglSyncEv ev)
{
    Sigil::instance().addEvent(ev);
}


/* XXX unimplemented */
//void SGLnotifyCF(SglCFEv ev)
//{
//}


////////////////////////////////////////////////////////////
// Frontend/Backend Registration
////////////////////////////////////////////////////////////
void Sigil::registerBackend(ToolName toolname, BackendFactory factory)
{
    std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);

    if (backend_registry.find(toolname) == backend_registry.cend())
    {
        /* initialize with empty parser and exit */
        backend_registry[toolname] = std::make_tuple(factory, [](Args) {}, []() {});
    }
    else
    {
        std::get<0>(backend_registry[toolname]) =  factory;
    }
}


void Sigil::registerParser(ToolName toolname, Parser parser)
{
    std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);

    if (backend_registry.find(toolname) == backend_registry.cend())
    {
        /* no way to initialize empty backend */
        SigiLog::fatal("Cannot register backend parser before registering backend");
    }
    else
    {
        std::get<1>(backend_registry[toolname]) =  parser;
    }
}


void Sigil::registerExit(ToolName toolname, Exit exit_routine)
{
    std::transform(toolname.begin(), toolname.end(), toolname.begin(), ::tolower);

    if (backend_registry.find(toolname) == backend_registry.cend())
    {
        /* no way to initialize empty backend */
        SigiLog::fatal("Cannot register backend parser before registering backend");
    }
    else
    {
        std::get<2>(backend_registry[toolname]) =  exit_routine;
    }
}


void Sigil::registerFrontend(std::string frontend, FrontendStarter start)
{
    std::transform(frontend.begin(), frontend.end(), frontend.begin(), ::tolower);
    frontend_registry[frontend] = start;
}


////////////////////////////////////////////////////////////
// Argument Parsing
////////////////////////////////////////////////////////////
void Sigil::parseOptions(int argc, char *argv[])
{
    /* custom parsing, by order of the arguments */
    ArgGroup arg_group;

    /* Pass through args to frontend/backend. */
    arg_group.addGroup(frontend, false);
    arg_group.addGroup(backend, true);
    arg_group.addGroup(executable, true);
    arg_group.parse(argc, argv);

    /* The number of 'threads' Sigil2 will use */
    /* MDL20160805 Currently only valid with DynamoRIO frontend. 
     * This will cause 'n' event streams between Sigil2 and DynamoRIO
     * to be generated, and 'n' separate backend instances will
     * read from those event streams as separate threads */
    num_threads = 1;
    if (arg_group.getOpt(numthreads).empty() == false)
    {
        num_threads = stoi(arg_group.getOpt(numthreads));
        if (num_threads > 16 || num_threads < 1)
        {
            SigiLog::fatal("Invalid number of threads specified");
        }
    }

    /* check frontend */
    std::string frontend_name;

    if (arg_group.getGroup(frontend).empty() == false)
    {
        frontend_name = arg_group.getGroup(frontend)[0];
    }
    else /*set default*/
    {
        frontend_name = "valgrind"; //default
    }

    std::transform(frontend_name.begin(), frontend_name.end(), frontend_name.begin(), ::tolower);

    if (frontend_registry.find(frontend_name) != frontend_registry.cend())
    {
        start_frontend = [this, arg_group, frontend_name]() mutable
        {
            Sigil::Args args;
            if (arg_group.getGroup(frontend).size() > 1)
            {
                auto start = arg_group.getGroup(frontend).cbegin() + 1;
                auto end = arg_group.getGroup(frontend).cend();
                args = {start, end};
            }

            frontend_registry[frontend_name](arg_group.getGroup(executable),
                                             args,
                                             num_threads,
                                             instance_id);
        };
    }
    else
    {
        std::string frontend_error(" invalid frontend argument ");
        frontend_error.append(frontend_name).append("\n");

        frontend_error.append("\tAvailable frontends: ");

        for (auto p : frontend_registry)
        {
            frontend_error.append("\n\t").append(p.first);
        }

        SigiLog::fatal(frontend_error);
    }

    /* check backend */
    std::string backend_name = arg_group.getGroup(backend)[0];
    std::transform(backend_name.begin(), backend_name.end(), backend_name.begin(), ::tolower);

    if (backend_registry.find(backend_name) != backend_registry.cend())
    {
        /* send args to backend */
        Sigil::Args args;

        if (arg_group.getGroup(backend).size() > 1)
        {
            auto start = arg_group.getGroup(backend).cbegin() + 1;
            auto end = arg_group.getGroup(backend).cend();
            args = {start, end};
        }

        /* Register the backend
         *
         * Each backend thread creates a new
         * backend instance via 'create_backend' */
        parse_backend = [this, backend_name, args]()
        {
            std::get<1>(backend_registry[backend_name])(args);
        };
        create_backend = std::get<0>(backend_registry[backend_name]);
        exit_backend = std::get<2>(backend_registry[backend_name]);
    }
    else
    {
        std::string backend_error(" invalid backend argument ");
        backend_error.append(backend_name).append("\n");

        backend_error.append("\tAvailable backends: ");

        for (auto p : backend_registry)
        {
            backend_error.append("\n\t").append(p.first);
        }

        SigiLog::fatal(backend_error);
    }
}
