cmake_minimum_required(VERSION 3.8)
project(stgenparser)

# This project requires C++17 support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

# Build Release optimized version by default
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING
		"Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
		FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(PRISMSRC ../../../../)

set(SOURCES0 CompressedParserExample.cpp
	StgenCapnpParser.cpp
	${PRISMSRC}/Utils/PrismLog.cpp)

set(SOURCES1 UncompressedParserExample.cpp
	StgenCapnpParser.cpp
	${PRISMSRC}/Utils/PrismLog.cpp)

include_directories(${PRISMSRC})
include_directories(${PRISMSRC}/Backends/SynchroTraceGen)
include_directories(${PRISMSRC}/../third_party/spdlog/include)

add_executable(stgenparser_compressed ${SOURCES0})
add_executable(stgenparser_uncompressed ${SOURCES1})

# We need to link stdc++fs because gcc doesn't have it included by default yet
# Additionally libkj and libcapnp must be available (typically via a capnproto package)
target_link_libraries(stgenparser_compressed pthread z kj capnp stdc++fs)
target_link_libraries(stgenparser_uncompressed pthread z kj capnp stdc++fs)
