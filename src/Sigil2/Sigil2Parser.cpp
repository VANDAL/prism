#include "Sigil2Parser.hpp"

using SigiLog::warn;
using SigiLog::fatal;

////////////////////////////////////////////////////////
// Sigil2Parser
////////////////////////////////////////////////////////
constexpr char Sigil2Parser::frontendOption[];
constexpr char Sigil2Parser::backendOption[];
constexpr char Sigil2Parser::executableOption[];
constexpr char Sigil2Parser::numThreadsOption[];

Sigil2Parser::Sigil2Parser(int argc, char* argv[])
{
    parser.addGroup(frontendOption, false);
    parser.addGroup(backendOption, true);
    parser.addGroup(executableOption, true);
    parser.parse(argc, argv);
}


auto Sigil2Parser::threads() -> int
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


auto Sigil2Parser::backend() -> ToolTuple
{
    return tool(backendOption);
}


auto Sigil2Parser::frontend() -> ToolTuple
{
    return tool(frontendOption);
}


auto Sigil2Parser::executable() -> Args
{
    return parser.getGroup(executableOption);
}

#include <iostream>
auto Sigil2Parser::tool(const char* option) -> ToolTuple
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


////////////////////////////////////////////////////////
// ArgGroup
////////////////////////////////////////////////////////
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
