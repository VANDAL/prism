cmake_minimum_required(VERSION 2.8)
project(sigil2)

#C++11 support
if (CMAKE_VERSION VERSION_LESS "3.1")
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
	endif ()
else ()
	set (CMAKE_CXX_STANDARD 11)
endif ()

enable_testing( true )
include( CTest )

include_directories(include/whereami/src)
include_directories(include/optionparser-1.4/src)
include_directories(include/spdlog/include/spdlog)
include_directories(include/elfio-3.1)
include_directories(include/zstream-cpp)
include_directories(src)

set(SIGIL2_DIR src/Sigil2)

set(SOURCES
	include/whereami/src/whereami.c
	${SIGIL2_DIR}/Sigil.cpp
	${SIGIL2_DIR}/Plugins.cpp
	${SIGIL2_DIR}/InstrumentationIface.cpp
	${SIGIL2_DIR}/EventManager.cpp
	${SIGIL2_DIR}/main.cpp)
add_executable(sigil2 ${SOURCES})
target_link_libraries(sigil2 pthread)

set_target_properties(sigil2
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

#Plugin Backends
set(STGen src/SynchroTraceGen)
add_subdirectory(${STGen})
target_link_libraries(sigil2 STGen)

set(dummy src/Dummy)
add_subdirectory(${dummy})
target_link_libraries(sigil2 dummy)


#Frontends
set(FrontEnds ${SIGIL2_DIR}/FrontEnds)
add_subdirectory(${FrontEnds}/Sigrind)
target_link_libraries(sigil2 Sigrind)

#All
add_custom_target(frontends ALL
	DEPENDS sigil2 valgrind
	COMMENT "Building Sigil2 and all frontends")
