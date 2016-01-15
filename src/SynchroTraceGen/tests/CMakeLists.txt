set (SMTEST_SOURCES
	../ShadowMemory.cpp
	)

set (STEVENTTEST_SOURCES
	../LogHelpers.cpp
	../STEvent.cpp
	../EventHandlers.cpp
	../ShadowMemory.cpp
	)

include_directories(
	../../../include #catch
	../../           #project src
	../../Sigil2
	)

add_executable(SMTest ./ShadMemTest.cpp ${SMTEST_SOURCES}) 

add_executable(STGenEventTest ./STEventTest.cpp ${STEVENTTEST_SOURCES}) 

add_test(
		NAME shadmemtest 
		COMMAND SMTest 
		)

add_test(
		NAME steventtest 
		COMMAND STGenEventTest 
		)
