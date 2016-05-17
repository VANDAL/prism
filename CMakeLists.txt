cmake_minimum_required(VERSION 3.1)
project(sigil2)

#C++11 support
set(CMAKE_CXX_STANDARD 11)

# enable_testing( true )
# include( CTest )

set(FRONTENDS src/Frontends)
set(BACKENDS src/Backends)
include_directories(include/whereami/src)
include_directories(include/optionparser-1.4/src)
include_directories(include/spdlog/include)
include_directories(include/spdlog/include/spdlog)
include_directories(include/elfio-3.1)
include_directories(include/zlib/contrib/iostream3)
include_directories(src)
include_directories(${FRONTENDS})
include_directories(${BACKENDS})

set(SIGIL2_DIR src/Sigil2)

set(SOURCES
	include/whereami/src/whereami.c
	${SIGIL2_DIR}/Sigil.cpp
	${SIGIL2_DIR}/Plugins.cpp
	${SIGIL2_DIR}/Frontends.cpp
	${SIGIL2_DIR}/EventBuffer.cpp
	${SIGIL2_DIR}/EventManager.cpp
	${SIGIL2_DIR}/main.cpp)
add_executable(sigil2 ${SOURCES})
target_link_libraries(sigil2 pthread)
set_target_properties(sigil2
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Plugin Backends
add_subdirectory(${BACKENDS}/SynchroTraceGen)
target_link_libraries(sigil2 STGen)

add_subdirectory(${BACKENDS}/Dummy)
target_link_libraries(sigil2 dummy)

# Interface to Frontends
add_subdirectory(${FRONTENDS}/Sigrind)
target_link_libraries(sigil2 Sigrind)

add_subdirectory(${FRONTENDS}/DrSigil)
target_link_libraries(sigil2 DrSigil)

#All
add_custom_target(frontends ALL
	DEPENDS sigil2 valgrind
	COMMENT "Building Sigil2 and all frontends")
