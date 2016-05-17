# Build destinations
get_target_property(SIGIL2_BIN sigil2 RUNTIME_OUTPUT_DIRECTORY)
set(VGINST_DIR ${SIGIL2_BIN}/vg)
set(VGSRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/valgrind-3.11.0-Sigil2)

# Function wrapper for intercepting multithreading libs
# (pthread and openmp function calls)
add_library(sglwrapper SHARED sglwrapper.c)
target_include_directories(sglwrapper PUBLIC
	gcc-4.9.2
	valgrind-3.11.0-Sigil2
	valgrind-3.11.0-Sigil2/include)
target_compile_definitions(sglwrapper PUBLIC
	-DVGO_linux=1)
set_target_properties(sglwrapper
	PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(SOURCES
	Sigrind.cpp)
add_library(Sigrind STATIC ${SOURCES})
target_include_directories(Sigrind PUBLIC
	valgrind-3.11.0-Sigil2/include
	valgrind-3.11.0-Sigil2/VEX/pub)
add_dependencies(Sigrind sglwrapper)

# valgrind build
include(ExternalProject)
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
set(valgrind PARENT_SCOPE)
