# dynamorio frontend -- drsigil
set(SOURCES DrSigilFrontend.cpp)
add_library(DrSigil OBJECT ${SOURCES})

# DynamoRIO
set(DR_PATH ${SRC_FRONTENDS}/DrSigil)
execute_process(
	COMMAND git submodule update --init ${DR_PATH}/dynamorio-Sigil2
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
include(ExternalProject)
set(INSTALL_PREFIX ${PROJECT_BINARY_DIR}/bin/dr)
ExternalProject_Add(dynamorio
	PREFIX dr
# -- Download step ------------
# -- Update step --------------
# -- Configure step -----------
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dynamorio-Sigil2
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX}
	-DCMAKE_CXX_STANDARD=98
	-DDISABLE_WARNINGS=ON # WErrors with gcc-6+
	-DDEBUG=OFF
	-DBUILD_DOCS=OFF
	-DBUILD_DRSTATS=OFF
	-DBUILD_SAMPLES=OFF
	-DBUILD_TESTS=OFF
	CONFIGURE_ALWAYS 0
	
# -- Build step ---------------
# -- Install step -------------
	BUILD_ALWAYS 0) # change 'STEP_ALWAYS 1' when testing DynamoRIO changes

# HACK to silence 32+64-bit install check warnings:
#   https://github.com/DynamoRIO/dynamorio/issues/1758
# We assume Sigil2 is being run for the native platform,
# otherwise unsupported
ExternalProject_Add_Step(dynamorio post_install
	COMMAND touch CMakeCache.txt
	WORKING_DIRECTORY ${INSTALL_PREFIX}
	DEPENDEES install)

add_dependencies(DrSigil dynamorio)
