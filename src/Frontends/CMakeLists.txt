# All available frontend interfaces must be in
# AvailableFrontends.hpp

# Add Valgrind frontend
add_subdirectory(Sigrind)
set(FRONTEND_TARGETS $<TARGET_OBJECTS:Sigrind>)

# Add DynamoRIO frontend
add_subdirectory(DrSigil)
set(FRONTEND_TARGETS ${FRONTEND_TARGETS} $<TARGET_OBJECTS:DrSigil>)

# Static or random event injector
#add_subdirectory(${SRC_FRONTENDS}/Injector)
#target_link_libraries(sigil2 Injector)

add_library(frontends STATIC ${FRONTEND_TARGETS})
