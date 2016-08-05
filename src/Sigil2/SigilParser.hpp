#include <map>
#include "SigiLog.hpp"


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

constexpr char frontend[] = "frontend";
constexpr char backend[] = "backend";
constexpr char executable[] = "executable";
constexpr char numthreads[] = "num-threads";

class ArgGroup
{
    using Args = std::vector<std::string>;

    /* long opt -> args */
    std::map<std::string, Args> group_args;

    /* command line args that don't follow a group */
    std::map<std::string, std::string> args;

    const Args empty_group;
    Args required_groups;
    Args optional_groups;
    std::string prev_group;

  public:
    /* Add a long option to group args */
    auto addGroup(const std::string &group, bool required) -> void;

    /* Check an argv[] to see if it's been added as an
     * arg group. If it is a validly formed arg group,
     * begin grouping consecutive options under this group
     * and return true; otherwise return false.
     *
     * long_opt is to be in the form: "--long_opt=argument" */
    auto tryGroup(const std::string &arg) -> bool;
    auto addArg(const std::string &arg) -> void;

    /* returns an empty vector if group does not exist,
     * or if no arguments were parsed for the group */
    auto getGroup(const std::string &group) -> const Args&;

    /* returns an empty string if the option does not exist,
     * or if no arguments were parsed for the option */
    auto getOpt(const std::string &opt) -> const std::string&;

    auto parse(int argc, const char *const argv[]) -> bool;
    auto display_help() -> void;
};

