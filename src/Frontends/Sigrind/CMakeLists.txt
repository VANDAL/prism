# Build destinations
get_target_property(SIGIL2_BIN sigil2 RUNTIME_OUTPUT_DIRECTORY)
set(VGINST_DIR ${SIGIL2_BIN}/vg)
set(VGSRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/valgrind-Sigil2)

include_directories(
	gcc-4.9.2
	valgrind-Sigil2
	valgrind-Sigil2/include
	valgrind-Sigil2/VEX/pub)

# Function wrapper for intercepting multithreading libs
# (pthread and openmp function calls)
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

set(SOURCES Sigrind.cpp)
add_library(Sigrind STATIC ${SOURCES})
add_dependencies(Sigrind sglwrapper-shared sglwrapper-static)

# valgrind build
include(ExternalProject)
execute_process(
	COMMAND git submodule update --init ${VGSRC_DIR}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ExternalProject_Add(valgrind
	PREFIX vg

	# CONFIGURE STEP
	SOURCE_DIR ${VGSRC_DIR}
	CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} ${VGSRC_DIR}/configure --prefix=${VGINST_DIR} --silent
	
	# BUILD STEP
	BUILD_COMMAND $(MAKE)

	# INSTALL STEP
	INSTALL_COMMAND $(MAKE) install > ${PROJECT_BINARY_DIR}/make_install_valgrind.log)

# pre-configure setup
ExternalProject_Add_Step(valgrind autogen
	COMMAND ./autogen.sh
	WORKING_DIRECTORY ${VGSRC_DIR}
	DEPENDERS configure)

add_dependencies(Sigrind valgrind)
