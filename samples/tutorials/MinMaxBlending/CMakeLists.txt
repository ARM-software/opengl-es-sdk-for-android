set(sources jni/Native.cpp jni/Matrix.cpp jni/Shader.cpp jni/Texture.cpp jni/Timer.cpp)
get_filename_component(sample ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_sample_gles3(${sample} "${sources}")

