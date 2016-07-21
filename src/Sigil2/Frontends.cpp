#include "Sigil.hpp"
#include "Frontends/Sigrind/Sigrind.hpp"
#include "Frontends/DrSigil/DrSigil.hpp"

namespace sgl
{

struct Frontend
{
    Frontend(std::string name, Sigil::FrontendStarter start)
    {
        Sigil::instance().registerFrontend(name, start);
    }
};

namespace
{
Frontend valgrind("valgrind", Sigrind::start);
Frontend dynamorio("dynamorio", DrSigil::start);
};

};
