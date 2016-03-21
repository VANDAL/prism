#ifndef SGL_PLUGINS_H
#define SGL_PLUGINS_H

#include "Sigil.hpp"

#define SGL_STRINGIFY(x) #x
#define SGL_STRINGIFYX(x) SGL_STRINGIFY(x)
#define SGL_PPCAT(x,y) x##y
#define SGL_PPCATX(x,y) SGL_PPCAT(x,y)

#define SIGIL_REGISTER(name) \
		struct SGL_PPCATX(tool_,name){ \
			std::string tmp; \
			void SGL_PPCATX(init_,name)(); \
			SGL_PPCATX(tool_,name)() { \
				tmp = SGL_STRINGIFY(name); \
				SGL_PPCATX(init_,name)(); \
			} \
		} SGL_PPCATX(name,_); \
		void SGL_PPCATX(tool_,name)::SGL_PPCATX(init_,name)()

#define BACKEND(backend) \
	Sigil::instance().registerBackend(tmp, []()->std::shared_ptr<Backend> { \
			return std::make_shared<backend>();} );

#define PARSER(parser) \
	Sigil::instance().registerParser(tmp, parser);

#define EXIT(exit_routine) \
	Sigil::instance().registerExit(tmp, exit_routine);



/* XXX unimplemented */
#define USAGE(func) Sigil::instance().registerToolUsage(func)

#endif
