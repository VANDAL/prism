#ifndef SGL_INJ_H
#define SGL_INJ_H

#include "Sigil2/Sigil.hpp"

/* Artificial injection of events for performance profiling */

namespace Injector
{

auto start(const std::vector<std::string> &user_exec,
           const std::vector<std::string> &args,
           const uint16_t num_threads,
           const std::string &instance_id
           ) -> void;

};

#endif
