set(SOURCES PerfPTFrontend.cpp)
add_library(PerfPT OBJECT ${SOURCES})
target_compile_definitions(PerfPT PRIVATE $<$<BOOL:${PERF_ENABLE}>:PERF_ENABLE>)


###############################################################################
if (${PERF_ENABLE})

execute_process(COMMAND grep -c intel_pt /proc/cpuinfo OUTPUT_VARIABLE INTEL_PT_SUPPORT)
if (${INTEL_PT_SUPPORT} EQUAL 0)
	message(FATAL_ERROR "Intel Process Trace not supported on this machine")
endif (${INTEL_PT_SUPPORT} EQUAL 0)

# Build destinations
get_target_property(SIGIL2_BIN sigil2 RUNTIME_OUTPUT_DIRECTORY)

include(ExternalProject)

###############################################################################
# xed library needed to decode instructions in modified sigil2 perf
set(XED_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/xed)
set(MBUILD_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/mbuild)

ExternalProject_Add(xed
	PREFIX xed

	# DOWNLOAD STEP
	DOWNLOAD_COMMAND git submodule update --init ${XED_SRCDIR} &&
					 git submodule update --init ${MBUILD_SRCDIR}

	# CONFIGURE STEP
	CONFIGURE_COMMAND ""

	# BUILD STEP
	BUILD_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/xed/mfile.py

	# INSTALL STEP
	INSTALL_COMMAND ""
	)

###############################################################################
set(PERF_DESTDIR ${SIGIL2_BIN}/perf)
set(LINUX_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/linux)
set(PERF_SRCDIR ${LINUX_SRCDIR}/tools/perf)
set(PATCH ${CMAKE_CURRENT_SOURCE_DIR}/gitpatches/linux.patch)

ExternalProject_Add(perf
	PREFIX perfpt

	# DOWNLOAD STEP
	DOWNLOAD_COMMAND git submodule update --init ${LINUX_SRCDIR}

	# PATCH STEP
	PATCH_COMMAND git -C ${LINUX_SRCDIR} apply --whitespace=nowarn ${PATCH}

	# CONFIGURE STEP
	CONFIGURE_COMMAND ""

	# BUILD STEP
	BUILD_COMMAND $(MAKE) -C ${PERF_SRCDIR}

	# INSTALL STEP
	INSTALL_COMMAND DESTDIR=${PERF_DESTDIR} $(MAKE) -C ${PERF_SRCDIR} install
	)

###############################################################################

add_dependencies(perf xed)
add_dependencies(PerfPT perf)

endif (${PERF_ENABLE})
