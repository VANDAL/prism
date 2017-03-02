# SynchroTraceGen Backend
set(SOURCES
	EventHandlers.cpp
	STEvent.cpp
	${CMAKE_SOURCE_DIR}/include/zlib/contrib/iostream3/zfstream.cc)
add_library(STGen STATIC ${SOURCES})
target_link_libraries(STGen z)
target_compile_options(STGen PUBLIC
					   "$<$<AND:$<CONFIG:RELEASE>,$<COMPILE_LANGUAGE:CXX>>:${SIGIL2_RELEASE_FLAGS}>")
add_subdirectory(tests)
