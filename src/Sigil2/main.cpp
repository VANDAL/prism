#include "OptionParser.hpp"
#include "EventManager.hpp"

int main(int argc, char* argv[])
{
	sgl::Clo clo(argc, argv);
	clo.parse();

	sgl::EventManager::instance().finish();
}
