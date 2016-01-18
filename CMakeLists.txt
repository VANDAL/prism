cmake_minimum_required(VERSION 3.0)
project(Sigil2)
set(VALGRIND_FLAGS "-DVGP_amd64_linux -DVGO_linux -DVGA_amd64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VALGRIND_FLAGS} -std=c++11 -rdynamic -lpthread -ldl")

enable_testing( true )
include( CTest )

#add_subdirectory(src)
#add_library(sigil2 SHARED ${SOURCES})

#TODO how to structure nicely/idiomatically
set(SIGIL2_DIR ./src/Sigil2)
set(STGEN_DIR ./src/SynchroTraceGen)

set(SOURCES)

#Sigil2
set(SOURCES
	${SOURCES}
	${SIGIL2_DIR}/InstrumentationIface.cpp
	${SIGIL2_DIR}/OptionParser.cpp
	${SIGIL2_DIR}/EventManager.cpp
	${SIGIL2_DIR}/Logger.cpp

	${SIGIL2_DIR}/main.cpp
	)

#Valgrind Frontend
set(SOURCES
	${SOURCES}
	${SIGIL2_DIR}/sigrind-listener.c
	${SIGIL2_DIR}/msg-builder.c
	)

set_source_files_properties( 
	${SIGIL2_DIR}/sigrind-listener.c 
	${SIGIL2_DIR}/msg-builder.c
	PROPERTIES LANGUAGE CXX )

#SynchroTraceGen Backend
set(SOURCES
	${SOURCES}
	${STGEN_DIR}/EventHandlers.cpp
	${STGEN_DIR}/ShadowMemory.cpp
	${STGEN_DIR}/LogHelpers.cpp
	${STGEN_DIR}/STEvent.cpp
	)

include_directories(src)
include_directories(${SIGIL2_DIR})
include_directories(src/Sigil2/valgrind-3.11.0-Sigil2/include)
include_directories(src/Sigil2/valgrind-3.11.0-Sigil2/VEX/pub)
include_directories(include/Catch/include)
include_directories(include/spdlog/include/spdlog)

add_executable(sigil ${SOURCES})

#test directories should have their own CMakeLists.txt
#add_subdirectory(./src/SynchroTraceGen/tests)
