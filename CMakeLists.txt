cmake_minimum_required(VERSION 3.0)
project(Sigil2)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread")

enable_testing( true )
include( CTest )

#add_subdirectory(src)
#add_library(sigil2 SHARED ${SOURCES})

#TODO how to structure nicely/idiomatically
set(SIGIL2_DIR ./src/Sigil2)
set(STGEN_DIR ./src/SynchroTraceGen)

set(SOURCES)
set(SOURCES
	${SOURCES}
	${SIGIL2_DIR}/InstrumentationIface.cpp
	${SIGIL2_DIR}/OptionParser.cpp
	${SIGIL2_DIR}/EventManager.cpp
	${SIGIL2_DIR}/Logger.cpp
	${SIGIL2_DIR}/Plugins.cpp

	${SIGIL2_DIR}/ScaleTest.cpp
	)

set(SOURCES
	${SOURCES}
	${STGEN_DIR}/EventHandlers.cpp
	${STGEN_DIR}/ShadowMemory.cpp
	${STGEN_DIR}/LogHelpers.cpp
	${STGEN_DIR}/STEvent.cpp
	)

include_directories(src)
include_directories(include/Catch/include)
include_directories(include/spdlog/include/spdlog)

add_executable(sigil ${SOURCES})

#test directories should have their own CMakeLists.txt
add_subdirectory(./src/SynchroTraceGen/tests)
