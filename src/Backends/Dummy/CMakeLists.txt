set(SOURCES
	Dummy.cpp
	)
add_library(dummy STATIC ${SOURCES})
set_property(TARGET dummy PROPERTY CXX_STANDARD 11)
