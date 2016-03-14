#ifndef SGL_PLUGINS_H
#define SGL_PLUGINS_H

#include "Sigil.hpp"

#define SGL_STRINGIFY(x) #x
#define SGL_STRINGIFYX(x) SGL_STRINGIFY(x)
#define SGL_PPCAT(x,y) x##y
#define SGL_PPCATX(x,y) SGL_PPCAT(x,y)

#define SIGIL_REGISTER(name) static void SGL_PPCATX(init_,name)(); \
		struct SGL_PPCATX(tool_,name){ SGL_PPCATX(tool_,name)() { \
			tmp = SGL_STRINGIFY(name); SGL_PPCATX(init_,name)(); } \
		} SGL_PPCATX(_,name); \
		static void SGL_PPCATX(init_,name)() \

std::string tmp;
#define EVENT_HANDLER(func) Sigil::instance().registerEventHandler(tmp, func)
#define USAGE(func) Sigil::instance().registerToolUsage(tmp, func)
#define PARSER(func) Sigil::instance().registerToolParser(tmp, func)
#define FINISH(func) Sigil::instance().registerToolFinish(tmp, func)

#endif
