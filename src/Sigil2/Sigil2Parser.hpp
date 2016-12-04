#ifndef SIGIL2_PARSER_H
#define SIGIL2_PARSER_H

#include <map>
#include "SigiLog.hpp"
#include "Backends.hpp"
#include "Frontends.hpp"


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
    using Args = std::vector<std::string>;
  public:
    ArgGroup()  = default;
    ~ArgGroup() = default;

    /* Add a long option to group args */
    auto addGroup(const std::string &group, bool required) -> void;

    /* Check an argv[] to see if it's been added as an
     * arg group. If it is a validly formed arg group,
     * begin grouping consecutive options under this group
     * and return true; otherwise return false.
     *
     * long_opt is to be in the form: "--long_opt=argument" */
    auto tryGroup(const std::string& arg) -> bool;
    auto addArg(const std::string& arg) -> void;

    auto getGroup(const std::string& group) const -> Args;
    auto getOpt(const std::string& opt) const -> std::string;

    auto parse(int argc, char* argv[]) -> bool;
    auto display_help() -> void;

  private:
    /* long opt -> args */
    std::map<std::string, Args> group_args;

    /* command line args that don't follow a group */
    std::map<std::string, std::string> args;

    const Args empty_group;
    Args required_groups;
    Args optional_groups;
    std::string prev_group;
};


class Sigil2Parser
{
    using ToolTuple = std::pair<std::string, std::vector<std::string>>;
  public:
    Sigil2Parser(int argc, char* argv[]);
    ~Sigil2Parser() = default;

    auto threads()    -> int;
    auto backend()    -> ToolTuple;
    auto frontend()   -> ToolTuple;
    auto executable() -> Args;

    /* get tool options in the form of a name and consecutive options:
     * --option=name -and -a --list -of --arbitrary -options */
    auto tool(const char* option) -> ToolTuple;

  private:
    ArgGroup parser;
    static constexpr char frontendOption[]   = "frontend";
    static constexpr char backendOption[]    = "backend";
    static constexpr char executableOption[] = "executable";
    static constexpr char numThreadsOption[] = "num-threads";
};

#endif
