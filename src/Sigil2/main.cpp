#include "OptionParser.hpp"
#include "EventManager.hpp"

int main(int argc, char* argv[])
{
	sgl::OptionParser parser(argc, argv);

	if ( parser.start_backend != nullptr && parser.start_frontend != nullptr )
	{
		parser.start_backend();
		parser.start_frontend();
		sgl::EventManager::instance().finish();
	}
}
