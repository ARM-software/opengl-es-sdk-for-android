file(GLOB sources jni/*.cpp)
get_filename_component(sample ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_sample(${sample} "${sources}")

