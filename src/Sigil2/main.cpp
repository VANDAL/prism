#include "spdlog.h"
#include "Sigil.hpp"

void prettyPrintSigil2()
{
	std::string title =
"                                                                 \n"
"     .M\"\"\"bgd `7MMF' .g8\"\"\"bgd `7MMF'`7MMF'                \n"
"     ,MI    \"Y   MM .dP'     `M   MM    MM                      \n"
"     `MMb.       MM dM'       `   MM    MM         pd*\"*b.      \n"
"       `YMMNq.   MM MM            MM    MM        (O)   j8       \n"
"     .     `MM   MM MM.    `7MMF' MM    MM      ,     ,;j9     \n"
"     Mb     dM   MM `Mb.     MM   MM    MM     ,M  ,-='        \n"
"     P\"Ybmmd\"  .JMML. `\"bmmmdPY .JMML..JMMmmmmMMM Ammmmmmm  \n"
"                                                               \n"
"                                                               \n";
	std::cerr << title;
}

int main(int argc, char* argv[])
{
	prettyPrintSigil2();

	/* Set up Sigil2 logging.
	 * TODO this needs to be cleaned up;
	 * an access to spdlog via spdlog::get with an invalid
	 * descriptor will fail with a segfault and may be confusing 
	 * to debug. Perhaps make a global variable somewhere... */
	std::map<std::string,std::string> ANSIcolors_fg =
	{
		{"black", "\033[30m"},
		{"red", "\033[31m"},
		{"green", "\033[32m"},
		{"yellow", "\033[33m"},
		{"blue", "\033[34m"},
		{"magenta", "\033[35m"},
		{"cyan", "\033[36m"},
		{"white", "\033[37m"},
		{"end", "\033[0m"}
	};
	spdlog::stdout_logger_st("sigil2-console");
	std::string header = "[Sigil2]";
	if (isatty(fileno(stdout))) header = "[" + ANSIcolors_fg["red"] + "Sigil2" + ANSIcolors_fg["end"] + "]";
	spdlog::get("sigil2-console")->set_pattern(header+" %v");


	try 
	{
		Sigil::instance().parseOptions(argc, argv);
		Sigil::instance().generateEvents();
	} 
	catch (std::exception& e) 
	{
		//TODO anything useful to do?
		std::terminate();
	}
}
