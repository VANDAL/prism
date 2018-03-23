cmake_minimum_required(VERSION 3.1.3)
project(prism)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING
		"Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
		FORCE)
endif(NOT CMAKE_BUILD_TYPE)

################################
# Testing Framework uses Catch #
################################
enable_testing(true)
include(CTest)

######################
# Include everything #
# TODO(cleanup)      #
######################
get_filename_component(SRC_CORE      src/Core      ABSOLUTE)
get_filename_component(SRC_FRONTENDS src/Frontends ABSOLUTE)
get_filename_component(SRC_BACKENDS  src/Backends  ABSOLUTE)
get_filename_component(THIRD_PARTY   third_party   ABSOLUTE)

execute_process(
	COMMAND git submodule update --init ${THIRD_PARTY}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
# Apply patch, or check if already applied
execute_process(
	COMMAND git apply --check ${CMAKE_SOURCE_DIR}/gitpatches/spdlog.patch
	WORKING_DIRECTORY ${THIRD_PARTY}/spdlog
	RESULT_VARIABLE PATCH_RESULT
	OUTPUT_QUIET
	ERROR_QUIET)
if (${PATCH_RESULT} EQUAL 0)
	execute_process(
		COMMAND git apply --whitespace=nowarn ${CMAKE_SOURCE_DIR}/gitpatches/spdlog.patch
		WORKING_DIRECTORY ${THIRD_PARTY}/spdlog)
else()
	execute_process(
		COMMAND git apply --check -R ${CMAKE_SOURCE_DIR}/gitpatches/spdlog.patch
		WORKING_DIRECTORY ${THIRD_PARTY}/spdlog
		RESULT_VARIABLE PATCH_RESULT)
	if (NOT ${PATCH_RESULT} EQUAL 0)
		message(FATAL_ERROR "could not git patch spdlog library")
	endif()
endif()

include_directories(${THIRD_PARTY}/Catch/include)
include_directories(${THIRD_PARTY}/whereami/src)
include_directories(${THIRD_PARTY}/spdlog/include)
include_directories(${THIRD_PARTY}/spdlog/include/spdlog)
include_directories(${THIRD_PARTY}/elfio-3.1)
include_directories(${THIRD_PARTY}/zlib/contrib/iostream3)
include_directories(${THIRD_PARTY}/capnproto/include)
include_directories(src)
include_directories(src/Utils)
include_directories(${SRC_FRONTENDS})
include_directories(${SRC_BACKENDS})


######################
# Optional Frontends #
######################
if(NOT DYNAMORIO_ENABLE)
	set(DYNAMORIO_ENABLE FALSE CACHE BOOL
		"Enable DynamoRIO frontend"
		FORCE)
endif(NOT DYNAMORIO_ENABLE)

if(NOT PERF_ENABLE)
	set(PERF_ENABLE FALSE CACHE BOOL
		"Enable perf frontend"
		FORCE)
endif(NOT PERF_ENABLE)


###############
# Build Prism #
###############
set(SOURCES
	${THIRD_PARTY}/whereami/src/whereami.c
	${SRC_CORE}/Backends.cpp
	${SRC_CORE}/Frontends.cpp
	${SRC_CORE}/Parser.cpp
	${SRC_CORE}/Config.cpp
	${SRC_CORE}/main.cpp)
add_executable(prism ${SOURCES})
target_link_libraries(prism pthread rt)
set_target_properties(prism
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

###################
# Plugin Backends #
###################
file(GLOB subdirs "src/Backends/*")
foreach(dir ${subdirs})
	set(PRISM_TOOLS_LIBS "")
	add_subdirectory(${dir})
	foreach(lib ${PRISM_TOOL_LIBS})
		target_link_libraries(prism ${lib})
	endforeach()
endforeach()

##########################
# Interface to Frontends #
##########################
add_subdirectory(${SRC_FRONTENDS})
target_link_libraries(prism frontends)
