# assume custom pytorch is already built
# TODO configure/setup here, instead of forcing the user to do so
set(SOURCES PyTorchFrontend.cpp)
add_library(PyTorchObserver OBJECT ${SOURCES})
target_compile_definitions(PyTorchObserver PRIVATE $<$<BOOL:${PYTORCH_ENABLE}>:PYTORCH_ENABLE>)
