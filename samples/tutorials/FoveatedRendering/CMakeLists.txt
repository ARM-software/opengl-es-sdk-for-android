set(sources jni/Native.cpp jni/model3d.cpp)
get_filename_component(sample ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_sample_gles3(${sample} "${sources}")

