#include <iterator>
#include <sstream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <cassert>

#include "ShMem.hpp"
#include "Sigil2/FrontEnds.hpp"

/* Sigil2's Valgrind frontend forks Valgrind off as a separate process;
 * Valgrind sends the frontend dynamic events from the application via shared memory */

namespace sgl
{
namespace sigrind
{
char* const* tokenizeOpts (const std::string &tmp_dir, const std::string &user_exec)
{
	assert( !user_exec.empty() );

	/* FIXME this does not account for quoted arguments with spaces
	 * Both whitespace and quote pairs should be delimiters */
	std::istringstream iss(user_exec);
	std::vector<std::string> tokens{
		std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>()};

	//                 program name + valgrind options + tmp_dir + user program options + null
	int vg_opts_size = 1            + 1                + 1       + tokens.size()        + 1;
	char** vg_opts = static_cast<char**>( malloc(vg_opts_size * sizeof(char*)) );

	int i = 0;
	vg_opts[i++] = strdup("valgrind");
	vg_opts[i++] = strdup("--tool=sigrind");
	vg_opts[i++] = strdup((std::string("--tmp-dir=") + tmp_dir).c_str());
	for (std::string token : tokens) 
	{
		vg_opts[i++] = strdup(token.c_str());
	}
	vg_opts[i] = nullptr;

	return vg_opts;
}

void start (
		const std::string &user_exec, 
		const std::string &sigrind_dir, 
		const std::string &tmp_dir
		) 
{
	assert ( !(user_exec.empty() || sigrind_dir.empty()) );

	std::string vg_exec = sigrind_dir + "/valgrind";

	// execvp() expects a const char* const*
	auto vg_opts = tokenizeOpts(tmp_dir, user_exec);

	// kickoff Valgrind
	if ( execvp(vg_exec.c_str(), vg_opts) == -1 )
	{
		std::perror("starting valgrind");
		throw std::runtime_error("Valgrind exec failed");
	}
}
}; //end namespace sigrind

void frontendSigrind (
		const std::string &user_exec, 
		const std::string &sigrind_dir, 
		const std::string &tmp_dir
		) 
{
	try
	{
		/* init shared memory interface */
		sigrind::ShMem sigrind_iface(tmp_dir);

		pid_t pid = fork();
		if ( pid >= 0 )
		{
			if ( pid == 0 )
			{
				sigrind::start(user_exec, sigrind_dir, tmp_dir);
			}
			else
			{
				sigrind_iface.readFromSigrind();
			}
		}
		else
		{
			std::perror("Sigrind frontend initialization");
			throw std::runtime_error("Sigrind fork failed");
		}
	}
	catch(std::exception& e)
	{
		throw e;
	}
}
}; //end namespace sgl
