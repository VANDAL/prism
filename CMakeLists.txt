cmake_minimum_required(VERSION 2.8)
project(sigil2)

####################################################
# TODO ONLY WORKING FOR 64-BIT X86 LINUX RIGHT NOW #
####################################################
set(VALGRIND_FLAGS "-DVGP_amd64_linux -DVGO_linux -DVGA_amd64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VALGRIND_FLAGS} -std=c++11 -DNDEBUG -O3 -rdynamic -lpthread -ldl")

enable_testing( true )
include( CTest )

include_directories(include/spdlog/include/spdlog)

include_directories(src)
set(SIGIL2_DIR src/Sigil2)
set(SOURCES
	${SIGIL2_DIR}/InstrumentationIface.cpp
	${SIGIL2_DIR}/OptionParser.cpp
	${SIGIL2_DIR}/EventManager.cpp
	${SIGIL2_DIR}/main.cpp)
add_executable(sigil2 ${SOURCES})

#Plugin Backends
set(STGen src/SynchroTraceGen)
add_subdirectory(${STGen})
target_link_libraries(sigil2 STGen)

#Frontends
set(FrontEnds ${SIGIL2_DIR}/FrontEnds)
add_subdirectory(${FrontEnds}/Sigrind)
target_link_libraries(sigil2 Sigrind)

#All
add_custom_target(frontends ALL
	DEPENDS sigil2 valgrind
	COMMENT "Building Sigil2 and all frontends")
