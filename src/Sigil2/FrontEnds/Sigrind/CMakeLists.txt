set(SOURCES
	MsgBuilder.cpp
	Sigrind.cpp)
add_library(Sigrind STATIC ${SOURCES})
target_include_directories(Sigrind PUBLIC 
	valgrind-3.11.0-Sigil2/include
	valgrind-3.11.0-Sigil2/VEX/pub)

add_custom_target(valgrind
	COMMAND ./autogen.sh && ./configure && $(MAKE)
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/valgrind-3.11.0-Sigil2)

set(valgrind PARENT_SCOPE)
