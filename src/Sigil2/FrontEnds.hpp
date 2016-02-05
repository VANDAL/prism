#ifndef SGL_FRONTENDS_H
#define SGL_FRONTENDS_H

/* All available frontends for Sigil */

#include <string>

namespace sgl
{
void frontendSigrind (
		const std::string& user_exec, 
		const std::string& sigrind_dir, 
		const std::string& tmp_dir
		);
};

#endif
