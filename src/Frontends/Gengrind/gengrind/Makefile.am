include $(top_srcdir)/Makefile.tool.am

EXTRA_DIST = docs/gn-manual.xml

#----------------------------------------------------------------------------
# gengrind-<platform>
#----------------------------------------------------------------------------

noinst_PROGRAMS  = gengrind-@VGCONF_ARCH_PRI@-@VGCONF_OS@
if VGCONF_HAVE_PLATFORM_SEC
noinst_PROGRAMS += gengrind-@VGCONF_ARCH_SEC@-@VGCONF_OS@
endif

GENGRIND_SOURCES_COMMON = gn_main.c\
						  gn_events.c\
						  gn_bb.c\
						  gn_callstack.c\
						  gn_jumps.c\
						  gn_fn.c\
						  gn_threads.c\
						  gn_crq.c\
						  gn_ipc.c\
						  gn_clo.c\
						  gn_debug.c

GENGRIND_CFLAGS_COMMON =  -I$(top_srcdir)/../../.. -I$(top_srcdir)/include -I$(top_srcdir)/VEX/pub

gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_SOURCES      = \
	$(GENGRIND_SOURCES_COMMON)
gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS       = \
	$(AM_CFLAGS_@VGCONF_PLATFORM_PRI_CAPS@) $(GENGRIND_CFLAGS_COMMON)
gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_PRI_CAPS@)
gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_PRI_CAPS@)
gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_PRI@ \
	$(LINK) \
	$(gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS) \
	$(gengrind_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS)

if VGCONF_HAVE_PLATFORM_SEC
gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_SOURCES      = \
	$(GENGRIND_SOURCES_COMMON)
gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS       = \
	$(AM_CFLAGS_@VGCONF_PLATFORM_SEC_CAPS@) $(GENGRIND_CFLAGS_COMMON)
gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_SEC_CAPS@)
gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_SEC_CAPS@)
gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_SEC@ \
	$(LINK) \
	$(gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS) \
	$(gengrind_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS)
endif


