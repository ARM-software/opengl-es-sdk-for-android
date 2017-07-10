# OpenGL ES SDK for Android

![OpenGL ES SDK for Android](https://user-images.githubusercontent.com/11390552/27276063-d0798c42-54d1-11e7-9695-940270e24d11.jpg)

## Introduction

The OpenGL ES Software Development Kit for Android is a collection of resources to help you build OpenGL ES applications
for a platform with a Mali GPU and an ARM processor. You can use it for creating new applications,
training, and exploration of implementation possibilities.

## Requirements

To build and run the OpenGL ES sample applications you will need:
-  An ARM based device with a Mali series GPU running Android.
-  [Android Studio](https://developer.android.com/studio/index.html).
-  The latest Android NDK, which should be downloaded automatically by Android Studio when you build the samples
    (alternatively, you can install it by following the instructions [here](https://developer.android.com/studio/projects/add-native-code.html#download-ndk)).

## License

The software is provided under an MIT license. Contributions to this project are accepted under the same license.

## Building

#### Build instructions

To build the sample applications:
- Open Android Studio.
- Click on "Open an existing Android Studio project" and point to the directory where you extracted the Android samples.
- Now you can select individual samples and run them directly on your target device.

#### Notes

  - You might be prompted to update or install the Gradle wrapper. Do so if asked.
  - You might be prompted to download and/or update Android SDK tools if Android Studio has not downloaded these before.
  - Under Tools -> Android -> SDK manager, install cmake, lldb and NDK components if these are not installed already.

#### Documentation
You can find the online documentation at https://arm-software.github.io/opengl-es-sdk-for-android/ 

or you can build the Doxygen documentation with `./build_documentation.sh`.
This requires Doxygen to be installed on your machine.
