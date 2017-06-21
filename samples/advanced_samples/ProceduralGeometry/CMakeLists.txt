file(GLOB sources jni/main.cpp)
get_filename_component(sample ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_sample_gles3(${sample} "${sources}")

