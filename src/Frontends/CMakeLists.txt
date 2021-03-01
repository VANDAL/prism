# All available frontend interfaces must be in
# AvailableFrontends.hpp

# Valgrind frontend
add_subdirectory(Gengrind)
set(FRONTEND_TARGETS $<TARGET_OBJECTS:Gengrind>)

# DynamoRIO frontend
add_subdirectory(DrSigil)
set(FRONTEND_TARGETS ${FRONTEND_TARGETS} $<TARGET_OBJECTS:DrSigil>)

# Perf frontend (intel_pt, only broadwell or newer)
add_subdirectory(PerfPT)
set(FRONTEND_TARGETS ${FRONTEND_TARGETS} $<TARGET_OBJECTS:PerfPT>)

# PyTorch/Caffe2 support (caffe2 only exist for now)
add_subdirectory(PyTorchObserver)
set(FRONTEND_TARGETS ${FRONTEND_TARGETS} $<TARGET_OBJECTS:PyTorchObserver>)

# Static or random event injector
#add_subdirectory(${SRC_FRONTENDS}/Injector)

set(SOURCES CleanupResources.cpp)
add_library(frontends STATIC ${FRONTEND_TARGETS} ${SOURCES})
