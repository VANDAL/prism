# Build destinations
get_target_property(SIGIL2_BIN sigil2 RUNTIME_OUTPUT_DIRECTORY)
set(VGINST_DIR ${SIGIL2_BIN}/vg)
set(VGBUILD_DIR ${PROJECT_BINARY_DIR}/vg-build)
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

# valgrind frontend -- sigrind
set(SOURCES
	Sigrind.cpp)
add_library(Sigrind STATIC ${SOURCES})
target_include_directories(Sigrind PUBLIC 
	valgrind-3.11.0-Sigil2/include
	valgrind-3.11.0-Sigil2/VEX/pub)
add_dependencies(Sigrind sglwrapper)

# Build valgrind for the user
# Required for valgrind frontend
add_custom_target(valgrind
	DEPENDS vg-build-dir
	DEPENDS autogen
	COMMAND CC=${CMAKE_C_COMPILER} ${VGSRC_DIR}/configure --prefix=${VGINST_DIR} --silent && $(MAKE) install > ${PROJECT_BINARY_DIR}/make_install_valgrind.log
	COMMENT "Make install valgrind > make_install_valgrind.log"
	WORKING_DIRECTORY ${VGBUILD_DIR})

add_custom_target(vg-build-dir
	COMMAND ${CMAKE_COMMAND} -E make_directory ${VGBUILD_DIR})

add_custom_target(autogen
	COMMAND ./autogen.sh 
	WORKING_DIRECTORY ${VGSRC_DIR})

set(valgrind PARENT_SCOPE)
