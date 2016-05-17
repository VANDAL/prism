# dynamorio frontend -- drsigil
set(SOURCES
	DrSigil.cpp)
add_library(DrSigil STATIC ${SOURCES})

# DynamoRIO
set(CMAKE_CXX_STANDARD 98)

include(ExternalProject)
set(INSTALL_PREFIX ${PROJECT_BINARY_DIR}/bin/dr)
ExternalProject_Add(dynamorio
	PREFIX dr

	# CONFIGURE STEP
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dynamorio
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}
	-DDEBUG=OFF
	-DBUILD_DOCS=OFF
	-DBUILD_DRSTATS=OFF
	-DBUILD_SAMPLES=ON
	-DBUILD_TESTS=OFF
	CONFIGURE_ALWAYS 0
	
	# BUILD STEP
	BUILD_ALWAYS 1)

# HACK to silence 32+64-bit install check warnings:
#   https://github.com/DynamoRIO/dynamorio/issues/1758
# We assume Sigil2 is being run for the native platform
ExternalProject_Add_Step(dynamorio post_install
	COMMAND touch CMakeCache.txt
	WORKING_DIRECTORY ${INSTALL_PREFIX}
	DEPENDEES install)

add_dependencies(DrSigil dynamorio)
