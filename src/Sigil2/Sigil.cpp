#include <algorithm>

#include "SigiLog.hpp"

#include "InstrumentationIface.h"
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
namespace
{

constexpr const char frontend[] = "frontend";
constexpr const char backend[] = "backend";
constexpr const char executable[] = "executable";

constexpr const char sigil2bin[] = "sigil2";
constexpr const char frontend_usage[] =  "--frontend=FRONTEND [options]";
constexpr const char backend_usage[] =   "--backend=BACKEND [options]";
constexpr const char executable_usage[] = "--executable=BINARY [options]";


[[noreturn]] void parse_error_exit(const std::string &msg)
{
    SigiLog::error("Error parsing arguments: " + msg);

    std::cout << "\nUSAGE:" << std::endl;
    std::cout << "    " << sigil2bin
              << " " << frontend_usage
              << " " << backend_usage
              << " " << executable_usage << std::endl << std::endl;

    exit(EXIT_FAILURE);
}


/* Sigil2 groups options together based on their position
 * in order to pass the option group to the frontend
 * instrumentation, the backend analysis, or the executable
 *
 * Most parsers have limited or confusing support for
 * order of non-option arguments in relation to option
 * arguments, including getopt
 *
 * Only allow long opts to avoid ambiguities.
 * Additionally imposes the constraint that the frontend,
 * backend, and executable cannot have any options that match */
class ArgGroup
{
    /* long opt -> args */
    std::map<std::string, Sigil::Args> args;
    std::vector<std::string> empty;
    std::string prev_opt;

  public:
    /* Add a long option to group args */
    void addGroup(const std::string &group)
    {
        if (group.empty() == true)
        {
            return;
        }

        args.emplace(group, Sigil::Args());
    }

    /* Check an argv[] to see if it's been added as an
     * arg group. If it is a validly formed arg group,
     * begin grouping consecutive options under this group
     * and return true; otherwise return false.
     *
     * long_opt is to be in the form: "--long_opt=argument" */
    bool tryGroup(const std::string &arg)
    {
        /* only long opts valid */
        if (arg.substr(0, 2).compare("--") != 0)
        {
            return false;
        }

        std::string rem(arg.substr(2));
        auto eqidx = rem.find('=');

        /* was this added? */
        if (args.find(rem.substr(0, eqidx)) == args.cend())
        {
            return false;
        }

        /* a valid arg group requires '=argument' */
        if (eqidx == std::string::npos || eqidx == rem.size() - 1)
        {
            parse_error_exit(std::string(arg).append(" missing argument"));
        }

        /* duplicate option groups not allowed */
        prev_opt = rem.substr(0, eqidx);

        if (args.at(prev_opt).empty() == false)
        {
            parse_error_exit(std::string(arg).append(" is duplicate option"));
        }

        /* initialize the group of args with this first argument */
        args.at(prev_opt).push_back(rem.substr(eqidx + 1));

        return true;
    }

    void addArg(const std::string &arg)
    {
        if (arg.empty() == true)
        {
            return;
        }

        /* the first argument must be an arg group */
        if (prev_opt.empty() == true)
        {
            parse_error_exit(std::string(arg).append(" is not valid here"));
        }

        args.at(prev_opt).push_back(arg);
    }

    const Sigil::Args &operator[](const std::string &group) const
    {
        if (args.find(group) == args.cend())
        {
            return empty;
        }
        else
        {
            return args.at(group);
        }
    }
};

}; //end namespace


void Sigil::parseOptions(int argc, char *argv[])
{
    ArgGroup arg_group;

    /* Pass through args to frontend/backend. */
    arg_group.addGroup(frontend);
    arg_group.addGroup(backend);
    arg_group.addGroup(executable);

    /* Parse loop */
    for (int optidx = 1; optidx < argc; ++optidx)
    {
        const char *curr_arg = argv[optidx];

        if (arg_group.tryGroup(curr_arg) == false)
        {
            arg_group.addArg(curr_arg);
        }
    }

    if (arg_group[backend].empty() == true || arg_group[executable].empty() == true)
    {
        parse_error_exit("missing required arguments");
    }

    /* check number of threads */
    //FIXME hardcode for testing
    num_threads = 1;

    /* check frontend */
    std::string frontend_name;

    if (arg_group[frontend].empty() == false)
    {
        frontend_name = arg_group[frontend][0];
    }
    else /*set default*/
    {
        frontend_name = "valgrind"; //default
    }

    std::transform(frontend_name.begin(), frontend_name.end(), frontend_name.begin(), ::tolower);

    if (frontend_registry.find(frontend_name) != frontend_registry.cend())
    {
        start_frontend = [this, arg_group, frontend_name]()
        {
            Sigil::Args args;

            if (arg_group[frontend].size() > 1)
            {
                auto start = arg_group[frontend].cbegin() + 1;
                auto end = arg_group[frontend].cend();
                args = {start, end};
            }

            frontend_registry[frontend_name](arg_group[executable],
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

        parse_error_exit(frontend_error);
    }

    /* check backend */
    std::string backend_name = arg_group[backend][0];
    std::transform(backend_name.begin(), backend_name.end(), backend_name.begin(), ::tolower);

    if (backend_registry.find(backend_name) != backend_registry.cend())
    {
        /* send args to backend */
        Sigil::Args args;

        if (arg_group[backend].size() > 1)
        {
            auto start = arg_group[backend].cbegin() + 1;
            auto end = arg_group[backend].cend();
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

        parse_error_exit(backend_error);
    }
}
