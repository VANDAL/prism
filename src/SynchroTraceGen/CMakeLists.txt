set(SOURCES
	EventHandlers.cpp
	ShadowMemory.cpp
	STEvent.cpp)
add_library(STGen STATIC ${SOURCES})
target_link_libraries(STGen z)
