file(GLOB sources jni/*.cpp)
get_filename_component(sample ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_sample(${sample} "${sources}")
if (${FILTER_TARGET} STREQUAL ${sample})
	# [Assimp Import Library]
	add_library(assimp SHARED IMPORTED)
	set_property(TARGET assimp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/assimp/libassimp.so)
	target_include_directories(${sample} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include)
	target_link_libraries(${sample} assimp)
	add_custom_command(TARGET ${sample} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/assimp/libassimp.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
	# [Assimp Import Library]
endif()

