set(SOURCES
	MsgBuilder.cpp
	SigrindListener.cpp)
add_library(Sigrind STATIC ${SOURCES})
target_include_directories(Sigrind PUBLIC 
	valgrind-3.11.0-Sigil2/include
	valgrind-3.11.0-Sigil2/VEX/pub)

add_custom_command(TARGET Sigrind
	POST_BUILD
	COMMAND ./autogen.sh
	COMMAND ./configure
	COMMAND $(MAKE)
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/valgrind-3.11.0-Sigil2)

