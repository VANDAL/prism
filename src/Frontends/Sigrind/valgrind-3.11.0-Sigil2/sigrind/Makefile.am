include $(top_srcdir)/Makefile.tool.am

EXTRA_DIST =  

#----------------------------------------------------------------------------
# Headers, etc
#----------------------------------------------------------------------------

pkginclude_HEADERS = callgrind.h

bin_SCRIPTS = 

noinst_HEADERS = \
	events.h \
	global.h \
	log_events.h


#----------------------------------------------------------------------------
# sigrind-<platform>
#----------------------------------------------------------------------------

noinst_PROGRAMS  = sigrind-@VGCONF_ARCH_PRI@-@VGCONF_OS@
if VGCONF_HAVE_PLATFORM_SEC
noinst_PROGRAMS += sigrind-@VGCONF_ARCH_SEC@-@VGCONF_OS@
endif

SIGRIND_SOURCES_COMMON = \
	bb.c \
	bbcc.c \
	callstack.c \
	clo.c \
	context.c \
	debug.c \
	events.c \
	fn.c \
	jumps.c \
	threads.c \
	log_events.c \
	sg_main.c 

# We sneakily include "PrimitveEnums.h" from Sigil2
SIGRIND_CFLAGS_COMMON =  -I$(top_srcdir)/../../.. -I$(top_srcdir)/include -I$(top_srcdir)/VEX/pub

sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_SOURCES      = \
	$(SIGRIND_SOURCES_COMMON)
sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS       = \
	$(AM_CFLAGS_@VGCONF_PLATFORM_PRI_CAPS@) $(SIGRIND_CFLAGS_COMMON)
sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_PRI_CAPS@)
sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_PRI_CAPS@)
sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_PRI@ \
	$(LINK) \
	$(sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS) \
	$(sigrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS)

if VGCONF_HAVE_PLATFORM_SEC
sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_SOURCES      = \
	$(SIGRIND_SOURCES_COMMON)
sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS       = \
	$(AM_CFLAGS_@VGCONF_PLATFORM_SEC_CAPS@) $(SIGRIND_CFLAGS_COMMON)
sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_SEC_CAPS@)
sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_SEC_CAPS@)
sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_SEC@ \
	$(LINK) \
	$(sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS) \
	$(sigrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS)
endif
