add_library(common-native STATIC
	src/Shader.cpp
	src/Text.cpp
	src/Texture.cpp
	src/ETCHeader.cpp
	src/Matrix.cpp
	src/JavaClass.cpp
	src/AndroidPlatform.cpp
	src/Timer.cpp)

target_include_directories(common-native PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/inc
	${CMAKE_CURRENT_SOURCE_DIR}/inc/mali)

target_compile_definitions(common-native PUBLIC GLES_VERSION=2)
target_link_libraries(common-native log GLESv2 EGL)

add_library(common-native-gles3 STATIC
	src/Shader.cpp
	src/Text.cpp
	src/Texture.cpp
	src/ETCHeader.cpp
	src/Matrix.cpp
	src/JavaClass.cpp
	src/AndroidPlatform.cpp
	src/Timer.cpp)

target_include_directories(common-native-gles3 PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/inc
	${CMAKE_CURRENT_SOURCE_DIR}/inc/mali)

target_compile_definitions(common-native-gles3 PUBLIC GLES_VERSION=3)
target_link_libraries(common-native-gles3 log GLESv3 EGL)
