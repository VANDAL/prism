######################
# Event Buffer Test  #
######################
set (SOURCES EventBufferTest.cpp ../../Utils/PrismLog.cpp ../CoreEventLoop.cpp)
add_executable(event_buffer_test ${SOURCES})
target_link_libraries(event_buffer_test pthread rt)
add_test(event_buffer_test event_buffer_test)
