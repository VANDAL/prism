# required CapnProto
set(CAPNPROTO_PATH ${SRC_BACKENDS}/SynchroTraceGen)
execute_process(
	COMMAND git submodule update --init ${CAPNPROTO_PATH}/capnproto
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include(ExternalProject)
ExternalProject_Add(capnproto
	PREFIX capnproto
# -- Download step ------------
# -- Update step --------------
# -- Configure step -----------
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/c++
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${THIRD_PARTY}/capnproto
	-DBUILD_TOOLS=OFF
	-DBUILD_TESTING=OFF
# -- Build step ---------------
# -- Install step -------------
	)
include_directories(${THIRD_PARTY}/capnproto/include)

# SynchroTraceGen Backend
set(SOURCES
	EventHandlers.cpp
	TextLogger.cpp
	CapnLogger.cpp
	STEvent.cpp
	STEventTrace.capnp.c++
	${THIRD_PARTY}/zlib/contrib/iostream3/zfstream.cc)
add_library(STGenCore STATIC ${SOURCES})

# Link CapnProto w/ SynchroTraceGen
set(KJ_LIB ${THIRD_PARTY}/capnproto/lib/libkj.a)
set(CAPNP_LIB ${THIRD_PARTY}/capnproto/lib/libcapnp.a)
set(STGEN_LIB ${CMAKE_CURRENT_BINARY_DIR}/libSTGen.a)
set(STGEN_LIB ${STGEN_LIB} PARENT_SCOPE) 
add_custom_target(STGen ALL
	COMMAND ar -x $<TARGET_FILE:STGenCore>
	COMMAND ar -x ${KJ_LIB}
	COMMAND ar -x ${CAPNP_LIB}
	COMMAND ar -qc ${STGEN_LIB} *.o)
add_dependencies(STGenCore capnproto)
add_dependencies(STGen STGenCore)

# tests
add_subdirectory(tests)
