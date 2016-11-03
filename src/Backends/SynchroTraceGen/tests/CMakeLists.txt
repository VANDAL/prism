cmake_minimum_required(VERSION 2.8)

######################
# Shadow Memory Test #
######################
set (SOURCES )
add_executable(shadow_memory_test ShadMemTest.cpp ${SOURCES})
target_link_libraries(shadow_memory_test pthread rt)
add_test(shadow_memory_test shadow_memory_test)

########################
# SynchroTraceGen Test #
########################
set (SOURCES ../EventHandlers.cpp ../STEvent.cpp
	${CMAKE_SOURCE_DIR}/include/zlib/contrib/iostream3/zfstream.cc)
add_executable(stgen_test STEventTest.cpp ${SOURCES})
target_link_libraries(stgen_test z pthread rt)
add_test(stgen_test stgen_test)
