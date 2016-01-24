#include "OptionParser.hpp"
#include "EventManager.hpp"

int main(int argc, char* argv[])
{
	sgl::OptionParser options(argc, argv);

	if ( options.start_backend != nullptr && options.start_frontend != nullptr )
	{
		options.start_backend();
		options.start_frontend();
		sgl::EventManager::instance().finish();
	}
}
