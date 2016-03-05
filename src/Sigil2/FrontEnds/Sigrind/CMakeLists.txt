set(SOURCES
	Sigrind.cpp)
add_library(Sigrind STATIC ${SOURCES})
target_include_directories(Sigrind PUBLIC 
	valgrind-3.11.0-Sigil2/include
	valgrind-3.11.0-Sigil2/VEX/pub)

get_target_property(SIGIL2_BIN sigil2 RUNTIME_OUTPUT_DIRECTORY)
set(VGINST_DIR ${SIGIL2_BIN}/vg)
set(VGBUILD_DIR ${PROJECT_BINARY_DIR}/vg-build)
set(VGSRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/valgrind-3.11.0-Sigil2)

add_custom_target(valgrind
	DEPENDS vg-build-dir
	DEPENDS autogen
	COMMAND CC=${CMAKE_C_COMPILER} ${VGSRC_DIR}/configure --prefix=${VGINST_DIR} --silent && $(MAKE) install
	WORKING_DIRECTORY ${VGBUILD_DIR})

add_custom_target(vg-build-dir
	COMMAND ${CMAKE_COMMAND} -E make_directory ${VGBUILD_DIR})

add_custom_target(autogen
	COMMAND ./autogen.sh 
	WORKING_DIRECTORY ${VGSRC_DIR})

set(valgrind PARENT_SCOPE)
