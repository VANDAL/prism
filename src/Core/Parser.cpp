#include "Parser.hpp"

using SigiLog::warn;
using SigiLog::fatal;

namespace sigil2
{

//-----------------------------------------------------------------------------
/** Parser **/
constexpr char Parser::frontendOption[];
constexpr char Parser::backendOption[];
constexpr char Parser::executableOption[];
constexpr char Parser::numThreadsOption[];
constexpr char Parser::timeOption[];

Parser::Parser(int argc, char* argv[])
{
    parser.addGroup(frontendOption, false);
    parser.addGroup(backendOption, true);
    parser.addGroup(executableOption, true);
    parser.parse(argc, argv);
}


auto Parser::threads() -> int
{
    /* The number of 'threads' Sigil2 will use */
    /* MDL20160805 Currently only valid with DynamoRIO frontend.
     * This will cause 'n' event streams between Sigil2 and DynamoRIO
     * to be generated, and 'n' separate backend instances will
     * read from those event streams as separate threads */

    int threads = 1;
    const auto threadsArg = parser.getOpt(numThreadsOption);
    if (threadsArg.empty() == false)
    {
        threads = stoi(threadsArg);

        if (threads > 16 || threads < 1)
            fatal("Invalid number of threads specified");
    }

    return threads;
}


auto Parser::backend() -> ToolTuple
{
    return tool(backendOption);
}


auto Parser::frontend() -> ToolTuple
{
    return tool(frontendOption);
}


auto Parser::executable() -> Args
{
    return parser.getGroup(executableOption);
}


auto Parser::timed() -> bool
{
    auto timeArg = parser.getOpt(timeOption);
    if (timeArg.empty() == false)
    {
        std::transform(timeArg.begin(), timeArg.end(), timeArg.begin(), ::tolower);
        if (timeArg == "on")
            return true;
        else if (timeArg == "off")
            return false;
        else
            fatal("Invalid 'time' option specified: " + timeArg);
    }

    return false;
}


auto Parser::tool(const char* option) -> ToolTuple
{
    const auto args = parser.getGroup(option);

    if (args.size() == 0)
        return {"", {}};

    auto name = args.front();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    auto start = args.cbegin() + 1;
    auto end = args.cend();

    return {name, {start, end}};
}


//-----------------------------------------------------------------------------
/** ArgGroup **/
auto ArgGroup::display_help() -> void
{
    std::string help;
    help = "Usage: \n";
    help += "                ";
    help += "sigil2 [options] ";

    for (const auto &option : optional_groups)
        help += "[--" + option + "=VALUE" + " [options]] ";

    for (const auto &option : required_groups)
        help += "--" + option + "=VALUE" + " [options] ";

    warn(help);
}


auto ArgGroup::addGroup(const std::string &group, bool required) -> void
{
    if (group.empty() == true)
    {
        return;
    }

    group_args.emplace(group, Args());

    if (required)
    {
        required_groups.emplace_back(group);
    }
    else
    {
        optional_groups.emplace_back(group);
    }
}


auto ArgGroup::tryGroup(const std::string &arg) -> bool
{
    /* only long opts valid */
    if (arg.substr(0, 2).compare("--") != 0)
    {
        return false;
    }

    std::string rem(arg.substr(2));
    auto eqidx = rem.find('=');

    /* if not a valid group */
    if (group_args.find(rem.substr(0, eqidx)) == group_args.cend())
    {
        return false;
    }

    /* a valid arg group requires '=argument' */
    if (eqidx == std::string::npos || eqidx == rem.size() - 1)
    {
        fatal(arg + " missing argument");
        return false;
    }

    /* duplicate option groups not allowed */
    prev_group = rem.substr(0, eqidx);
    if (group_args.at(prev_group).empty() == false)
    {
        fatal(arg + " is duplicate option");
    }

    /* initialize the group of args with this first argument */
    group_args.at(prev_group).push_back(rem.substr(eqidx + 1));

    return true;
}


auto ArgGroup::addArg(const std::string &arg) -> void
{
    if (arg.empty() == true)
    {
        return;
    }

    if (prev_group.empty() == false)
    {
        group_args.at(prev_group).push_back(arg);
    }
    else
    {
        /* only long opts valid */
        if (arg.substr(0, 2).compare("--") != 0)
        {
            warn("unrecognized option: " + arg);
            return;
        }

        std::string rem(arg.substr(2));
        auto eqidx = rem.find('=');

        /* a valid arg group requires '=argument' */
        if (eqidx == std::string::npos || eqidx == rem.size() - 1)
        {
            warn("unrecognized option: " + arg);
            return;
        }

        args[rem.substr(0, eqidx)] = rem.substr(eqidx+1);
    }
}


auto ArgGroup::getGroup(const std::string &group) const -> Args
{
    auto group_search = group_args.find(group);

    if (group_search == group_args.cend())
    {
        return std::vector<std::string>();
    }

    return group_search->second;
}


auto ArgGroup::getOpt(const std::string &opt) const -> std::string
{
    auto opt_search = args.find(opt);

    if (opt_search == args.cend())
    {
        return std::string();
    }

    return opt_search->second;
}


auto ArgGroup::parse(int argc, char* argv[]) -> bool
{
    for (int optidx = 1; optidx < argc; ++optidx)
    {
        const char *curr_arg = argv[optidx];

        if (tryGroup(curr_arg) == false)
        {
            addArg(curr_arg);
        }
    }

    for (const auto& group : required_groups)
    {
        if (group_args.at(group).empty())
        {
            display_help();
            fatal("--" + group + "= is missing");
        }
    }

    return true;
}

}; //end namespace sigil2
