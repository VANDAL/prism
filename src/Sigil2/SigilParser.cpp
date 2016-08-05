#ifndef SIGIL_PARSER_H
#define SIGIL_PARSER_H

#include "SigilParser.hpp"

auto ArgGroup::display_help() -> void
{
    std::string help;
    help = "Usage: \n";
    help += "                ";
    help += "sigil2 [options] ";
    for (const auto &option : optional_groups)
    {
        help += "[--" + option + "=VALUE" + " [options]] ";
    }
    for (const auto &option : required_groups)
    {
        help += "--" + option + "=VALUE" + " [options] ";
    }
    SigiLog::warn(help);
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
        SigiLog::fatal(arg + " missing argument");
        return false;
    }

    /* duplicate option groups not allowed */
    prev_group = rem.substr(0, eqidx);

    if (group_args.at(prev_group).empty() == false)
    {
        SigiLog::fatal(arg + " is duplicate option");
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
            SigiLog::warn("unrecognized option: " + arg);
            return;
        }

        std::string rem(arg.substr(2));
        auto eqidx = rem.find('=');

        /* a valid arg group requires '=argument' */
        if (eqidx == std::string::npos || eqidx == rem.size() - 1)
        {
            SigiLog::warn("unrecognized option: " + arg);
            return;
        }

        args[rem.substr(0, eqidx)] = rem.substr(eqidx+1);
    }
}


auto ArgGroup::getGroup(const std::string &group) -> const Args&
{
    assert (group_args.find(group) != group_args.cend());
    return group_args[group];
}


auto ArgGroup::getOpt(const std::string &opt) -> const std::string&
{
    assert (args.find(opt) != args.cend());
    return args[opt];
}


auto ArgGroup::parse(int argc, const char *const argv[]) -> bool
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
            SigiLog::fatal("--" + group + "= is missing");
        }
    }

    return true;
}

#endif
