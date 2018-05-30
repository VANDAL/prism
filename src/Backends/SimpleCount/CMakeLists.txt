set(SOURCES
	Handler.cpp)
add_library(SimpleCount STATIC ${SOURCES})

set(PRISM_TOOL_LINK_LIBS SimpleCount PARENT_SCOPE)
