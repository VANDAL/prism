set(SOURCES
	Handler.cpp
	SigilClassic.cpp)
add_library(SigilClassic STATIC ${SOURCES})

set(PRISM_TOOL_LIBS SigilClassic PARENT_SCOPE)
