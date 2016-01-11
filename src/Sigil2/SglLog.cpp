#include "SglLog.hpp"

#include <unordered_map>
#include <cassert>

namespace sgl
{
	/* spdlog documentation recommends saving logger pointers due 
	 * to costly locks when using spdlog::get */
	static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers;

	void logCreateFile(const char* const filename)
	{
		if ( spdlog::get(filename) == 0 )
		{
			spdlog::set_async_mode(4096);
			spdlog::create<spdlog::sinks::simple_file_sink_st>(filename, filename);
			spdlog::get(filename)->set_pattern("%v");

			loggers[filename] = spdlog::get(filename);
		}
	}

	void logToFile(const char* const filename, const char* const msg)
	{
		/* User expected to set up file with logCreateFile.
		 *
		 * Putting a check here is costly because certain back-ends,
		 * like SynchroTrace log frequently */
		assert( loggers.find(filename) != loggers.cend() );

		//this can be sped up by forcing the user to save the logging pointer
		loggers[filename]->info(msg);
	}
}
