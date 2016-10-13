cmake_minimum_required(VERSION 2.8)
project(sigil2)

####################################################
# C++11 support                                    #
# from http://stackoverflow.com/a/31010221/1371191 #
####################################################
macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall") 
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto") 

################################
# Testing Framework uses Catch #
################################
enable_testing(true)
include(CTest)

# http://stackoverflow.com/a/34900012/1371191
# NOT COMPATIBLE WITH version <2.8.12
# add_library(Catch INTERFACE)
# target_include_directories(Catch INTERFACE include/Catch/include)

###################################################
# Include everything                              #
# ...there's probably a cleaner way to do this... #
###################################################
get_filename_component(SRC_SIGIL2 src/Sigil2 ABSOLUTE)
get_filename_component(SRC_FRONTENDS src/Frontends ABSOLUTE)
get_filename_component(SRC_BACKENDS src/Backends ABSOLUTE)

include_directories(include/Catch/include)
include_directories(include/whereami/src)
include_directories(include/optionparser-1.4/src)
include_directories(include/spdlog/include)
include_directories(include/spdlog/include/spdlog)
include_directories(include/elfio-3.1)
include_directories(include/zlib/contrib/iostream3)
include_directories(src)
include_directories(${SRC_FRONTENDS})
include_directories(${SRC_BACKENDS})

################
# Build Sigil2 #
################
set(SOURCES
	include/whereami/src/whereami.c
	${SRC_SIGIL2}/Sigil.cpp
	${SRC_SIGIL2}/SigilParser.cpp
	${SRC_SIGIL2}/Plugins.cpp
	${SRC_SIGIL2}/Frontends.cpp
	${SRC_SIGIL2}/EventBuffer.cpp
	${SRC_SIGIL2}/EventManager.cpp
	${SRC_SIGIL2}/main.cpp)
add_executable(sigil2 ${SOURCES})
target_link_libraries(sigil2 pthread rt)
set_target_properties(sigil2
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

###################
# Plugin Backends #
###################
add_subdirectory(${SRC_BACKENDS}/SynchroTraceGen)
target_link_libraries(sigil2 STGen)

add_subdirectory(${SRC_BACKENDS}/SimpleCount)
target_link_libraries(sigil2 SimpleCount)

##########################
# Interface to Frontends #
##########################
# static or random event injector
add_subdirectory(${SRC_FRONTENDS}/Injector)
target_link_libraries(sigil2 Injector)

# valgrind DBI -- default
add_subdirectory(${SRC_FRONTENDS}/Sigrind)
target_link_libraries(sigil2 Sigrind)

# dynamorio DBI -- experimental
if(ENABLE_DRSIGIL)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_DRSIGIL")

	add_subdirectory(${SRC_FRONTENDS}/DrSigil)
	target_link_libraries(sigil2 DrSigil)
endif(ENABLE_DRSIGIL)
