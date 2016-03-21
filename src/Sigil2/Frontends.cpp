#include "Sigil.hpp"
#include "Frontends/Sigrind/Sigrind.hpp"

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
Frontend Sigrind("valgrind", Sigrind::start);
};

};
