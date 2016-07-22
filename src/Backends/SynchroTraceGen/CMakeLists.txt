# SynchroTraceGen Backend
set(SOURCES
	EventHandlers.cpp
	ShadowMemory.cpp
	STEvent.cpp
	${CMAKE_SOURCE_DIR}/include/zlib/contrib/iostream3/zfstream.cc)
add_library(STGen STATIC ${SOURCES})
target_link_libraries(STGen z)

add_subdirectory(tests)
