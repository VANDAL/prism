#ifndef SGL_PLUGINS_H
#define SGL_PLUGINS_H

/* See source file */

#define REGISTER_PREFIX SGLregister_
#define SGL_STRINGIFY(x) #x
#define SGL_STRINGIFYX(x) SGL_STRINGIFY(x)
#define SGL_PPCAT(x,y) x##y
#define SGL_PPCATX(x,y) SGL_PPCAT(x,y)
#define SIGIL_REGISTER(name) extern "C" void SGL_PPCATX(REGISTER_PREFIX, name)()

#endif
