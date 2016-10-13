#include "Sigil.hpp"
#include "Frontends/Injector/Injector.hpp"
#include "Frontends/Sigrind/Sigrind.hpp"

#ifdef ENABLE_DRSIGIL
#include "Frontends/DrSigil/DrSigil.hpp"
#endif

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
Frontend inject("inject", Injector::start);
Frontend valgrind("valgrind", Sigrind::start);

#ifdef ENABLE_DRSIGIL
Frontend dynamorio("dynamorio", DrSigil::start);
#endif
};

};
