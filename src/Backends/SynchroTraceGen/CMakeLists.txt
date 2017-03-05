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

# SynchroTraceGen Backend
set(SOURCES
	EventHandlers.cpp
	STEvent.cpp
	STEventTrace.capnp.c++
	${THIRD_PARTY}/zlib/contrib/iostream3/zfstream.cc)
add_library(STGen STATIC ${SOURCES})
target_link_libraries(STGen z
	${THIRD_PARTY}/capnproto/lib/libcapnp.a
	${THIRD_PARTY}/capnproto/lib/libkj.a)
target_compile_options(STGen PUBLIC
					   "$<$<AND:$<CONFIG:RELEASE>,$<COMPILE_LANGUAGE:CXX>>:${SIGIL2_RELEASE_FLAGS}>")
add_dependencies(STGen capnproto)

add_subdirectory(tests)
