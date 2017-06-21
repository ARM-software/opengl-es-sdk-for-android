file(GLOB sources jni/*.cpp jni/common/*.cpp)
get_filename_component(sample ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_sample_gles3(${sample} "${sources}")
if (${FILTER_TARGET} STREQUAL "${sample}")
	target_include_directories(${sample} PRIVATE jni/common)
endif()

