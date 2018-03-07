# Build destinations
get_target_property(SIGIL2_BIN sigil2 RUNTIME_OUTPUT_DIRECTORY)
set(VG_PREFIX vg)
set(VGINST_DIR ${SIGIL2_BIN}/${VG_PREFIX})
set(VGSRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/valgrind)
set(PATCH ${CMAKE_CURRENT_SOURCE_DIR}/gitpatches/sigrind.patch)

include_directories(
	valgrind
	valgrind/include
	valgrind/VEX/pub)

###############################################################################
execute_process(
	COMMAND git submodule update --init ${VGSRC_DIR}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Copy Prism-specific tools to valgrind
execute_process(
	COMMAND cp -rf gengrind valgrind
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_QUIET)
execute_process(
	COMMAND cp -rf sigrind valgrind
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_QUIET)

# Apply patch, or check if already applied
execute_process(
	COMMAND git apply --check ${PATCH}
	WORKING_DIRECTORY ${VGSRC_DIR}
	RESULT_VARIABLE PATCH_RESULT
	OUTPUT_QUIET
	ERROR_QUIET)
if (${PATCH_RESULT} EQUAL 0)
	execute_process(
		COMMAND git apply --whitespace=nowarn ${PATCH}
		WORKING_DIRECTORY ${VGSRC_DIR})
else()
	execute_process(
		COMMAND git apply --check -R ${PATCH}
		WORKING_DIRECTORY ${VGSRC_DIR}
		RESULT_VARIABLE PATCH_RESULT)
	if (NOT ${PATCH_RESULT} EQUAL 0)
		message(FATAL_ERROR "could not git patch valgrind")
	endif()
endif()

###############################################################################
# frontend interface
set(SOURCES SigrindFrontend.cpp)
add_library(Sigrind OBJECT ${SOURCES})

###############################################################################
# valgrind build
include(ExternalProject)
set(VG_PROJECT valgrind)
ExternalProject_Add(${VG_PROJECT}
	PREFIX ${VG_PREFIX}

	# CONFIGURE STEP
	SOURCE_DIR ${VGSRC_DIR}
	CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} ${VGSRC_DIR}/configure --prefix=${VGINST_DIR} --silent
	
	# BUILD STEP
	BUILD_COMMAND $(MAKE)

	# INSTALL STEP
	INSTALL_COMMAND $(MAKE) install > ${PROJECT_BINARY_DIR}/make_install_valgrind.log)

# need this when building the wrapper
set(VGBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/${VG_PREFIX}/src/${VG_PROJECT}-build)

# pre-configure setup
ExternalProject_Add_Step(valgrind autogen
	COMMAND ./autogen.sh
	WORKING_DIRECTORY ${VGSRC_DIR}
	DEPENDERS configure)

add_dependencies(Sigrind valgrind)

###############################################################################
# Function wrapper for intercepting multithreading libs
# (pthread and openmp function calls)
include_directories(
	gcc-4.9.2
	${VGBUILD_DIR})
add_library(sglwrapper-shared SHARED sglwrapper.c)
add_library(sglwrapper-static STATIC sglwrapper.c)
set_target_properties(sglwrapper-shared
	PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
	OUTPUT_NAME sglwrapper)
set_target_properties(sglwrapper-static
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
	OUTPUT_NAME sglwrapper)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
	set(VG_ARCH VGA_amd64)
elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
	set(VG_ARCH VGA_arm64)
else()
	message(FATAL_ERROR "Unsupported architecture")
endif()
target_compile_definitions(sglwrapper-shared PUBLIC -DVGO_linux=1 -D${VG_ARCH})
target_compile_definitions(sglwrapper-static PUBLIC -DVGO_linux=1 -D${VG_ARCH})

add_dependencies(sglwrapper-static valgrind)
add_dependencies(sglwrapper-shared valgrind)
add_dependencies(Sigrind sglwrapper-shared sglwrapper-static)
