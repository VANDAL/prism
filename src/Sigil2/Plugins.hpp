#ifndef SGL_PLUGINS_H
#define SGL_PLUGINS_H

#include "Sigil.hpp"

#define SGL_STRINGIFY(x) #x
#define SGL_STRINGIFYX(x) SGL_STRINGIFY(x)
#define SGL_PPCAT(x,y) x##y
#define SGL_PPCATX(x,y) SGL_PPCAT(x,y)

#define SIGIL_REGISTER(name) static void SGL_PPCATX(init_,name)(); \
		struct SGL_PPCATX(tool_,name){ SGL_PPCATX(tool_,name)() { \
			Sigil::instance().registerBackend(SGL_STRINGIFY(name), SGL_PPCATX(init_,name));} \
		} SGL_PPCATX(name,_); \
		static void SGL_PPCATX(init_,name)() \

#define EVENT_HANDLER(func) Sigil::instance().registerEventHandler(func)
#define PARSER(func) Sigil::instance().registerToolParser(func)
#define FINISH(func) Sigil::instance().registerToolFinish(func)

/* XXX unimplemented */
#define USAGE(func) Sigil::instance().registerToolUsage(func)

#endif
