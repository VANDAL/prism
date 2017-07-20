# dynamorio frontend -- drsigil
set(SOURCES DrSigilFrontend.cpp)
add_library(DrSigil OBJECT ${SOURCES})
target_compile_definitions(DrSigil PRIVATE $<$<BOOL:${DYNAMORIO_ENABLE}>:DYNAMORIO_ENABLE>)


###############################################################################
if (${DYNAMORIO_ENABLE})

# DynamoRIO
set(DR_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/dynamorio)
set(DR_INSTALLDIR ${PROJECT_BINARY_DIR}/bin/dr)
set(PATCH ${CMAKE_CURRENT_SOURCE_DIR}/gitpatches/drsigil.patch)

execute_process(
	COMMAND git submodule update --init ${DR_SRCDIR}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
# Apply patch, or check if already applied
execute_process(
	COMMAND git apply --check ${PATCH}
	WORKING_DIRECTORY ${DR_SRCDIR}
	RESULT_VARIABLE PATCH_RESULT
	OUTPUT_QUIET
	ERROR_QUIET)
if (${PATCH_RESULT} EQUAL 0)
	execute_process(
		COMMAND git apply --whitespace=nowarn ${PATCH}
		WORKING_DIRECTORY ${DR_SRCDIR})
else()
	execute_process(
		COMMAND git apply --check -R ${PATCH}
		WORKING_DIRECTORY ${DR_SRCDIR}
		RESULT_VARIABLE PATCH_RESULT)
	if (NOT ${PATCH_RESULT} EQUAL 0)
		message(FATAL_ERROR "could not git patch dynamorio")
	endif()
endif()

include(ExternalProject)
ExternalProject_Add(dynamorio
	PREFIX dr

# -- DOWNLOAD STEP ------------
# -- PATCH STEP ------------

# -- UPDATE STEP --------------
# -- CONFIGURE STEP -----------
	SOURCE_DIR ${DR_SRCDIR}
	INSTALL_DIR ${DR_INSTALLDIR}
	# Required: https://stackoverflow.com/a/32473121
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${DR_INSTALLDIR}
	-DCMAKE_CXX_STANDARD=98
	-DDISABLE_WARNINGS=ON # WErrors with gcc-6+
	-DDEBUG=OFF
	-DBUILD_DOCS=OFF
	-DBUILD_DRSTATS=OFF
	-DBUILD_SAMPLES=OFF
	-DBUILD_TESTS=OFF
	CONFIGURE_ALWAYS 0
	
# -- BUILD STEP ---------------
# -- INSTALL STEP -------------

	BUILD_ALWAYS 0) # change 'STEP_ALWAYS 1' when testing DynamoRIO changes

# HACK to silence 32+64-bit install check warnings:
#   https://github.com/DynamoRIO/dynamorio/issues/1758
# We assume Sigil2 is being run for the native platform,
# otherwise unsupported
ExternalProject_Add_Step(dynamorio post_install
	COMMAND touch CMakeCache.txt
	WORKING_DIRECTORY ${DR_INSTALLDIR}
	DEPENDEES install)

add_dependencies(DrSigil dynamorio)

endif (${DYNAMORIO_ENABLE})
