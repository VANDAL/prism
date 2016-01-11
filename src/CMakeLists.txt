set (SOURCES
	${SOURCES}
	hello
	PARENT_SCOPE
	)
message(${SOURCES})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SynchroTraceGen)

#set(SOURCES ${SOURCES})

message(${SOURCES})


#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SynchroTraceGen)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Sigil2)
