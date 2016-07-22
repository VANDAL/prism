cmake_minimum_required(VERSION 2.8)

set (SOURCES ../ShadowMemory.cpp)
add_executable(shadow_memory_test ShadMemTest.cpp ${SOURCES}) 
target_link_libraries(shadow_memory_test Catch pthread)
add_test(shadow_memory_test shadow_memory_test)

# set (STEVENTTEST_SOURCES
# 	../STEvent.cpp
# 	../EventHandlers.cpp
# 	../ShadowMemory.cpp
# 	)

# add_executable(synchrotrace_event_test ./STEventTest.cpp ${STEVENTTEST_SOURCES}) 

# add_executable(synchrotracegen_test ./STEventTest.cpp ${STEVENTTEST_SOURCES}) 

# add_test(NAME steventtest COMMAND STGenEventTest)
