######################
# Shadow Memory Test #
######################
set (SOURCES ShadMemTest.cpp ../../../Utils/PrismLog.cpp)
add_executable(shadow_memory_test ShadMemTest.cpp ${SOURCES})
target_link_libraries(shadow_memory_test pthread rt)
add_test(shadow_memory_test shadow_memory_test)

######################
# Barrier Merge Test #
######################
set (SOURCES BarrierMergeTest.cpp)
add_executable(barrier_merge_test BarrierMergeTest.cpp ${SOURCES})
target_link_libraries(barrier_merge_test rt)
add_test(barrier_merge_test barrier_merge_test)
