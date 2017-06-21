function(add_sample_inner TARGET SOURCES)
	# For Android, always emit libnative.so since we only build one sample per APK.
	# Otherwise, we want to control our output folder so we can pick up assets without any problems.
	add_library(${TARGET} SHARED ${SOURCES})
	target_link_libraries(${TARGET} common-native)
	set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_NAME Native)
	target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/jni)
endfunction()

function(add_sample_inner_gles3 TARGET SOURCES)
	# For Android, always emit libnative.so since we only build one sample per APK.
	# Otherwise, we want to control our output folder so we can pick up assets without any problems.
	add_library(${TARGET} SHARED ${SOURCES})
	target_link_libraries(${TARGET} common-native-gles3)
	set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_NAME Native)
	target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/jni)
endfunction()

function(add_sample TARGET SOURCES)
	if (${FILTER_TARGET} STREQUAL ${TARGET})
		add_sample_inner(${TARGET} "${SOURCES}")
	endif(${FILTER_TARGET} STREQUAL ${TARGET})
endfunction()

function(add_sample_gles3 TARGET SOURCES)
	if (${FILTER_TARGET} STREQUAL ${TARGET})
		add_sample_inner_gles3(${TARGET} "${SOURCES}")
	endif(${FILTER_TARGET} STREQUAL ${TARGET})
endfunction()
