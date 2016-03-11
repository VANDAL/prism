#ifndef SGL_FRONTENDS_H
#define SGL_FRONTENDS_H

#include <string>
#include <vector>

/* All available frontends for Sigil */
namespace sgl
{
void frontendSigrind(
		const std::vector<std::string> &user_exec, 
		const std::vector<std::string> &args);
};

#endif
