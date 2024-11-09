# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.10.2)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("pthread_key_detect")
SET(SOURCE_DIR src/main/cpp)
# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${SOURCE_DIR}/pthread_key_detect.cpp
        ${SOURCE_DIR}/nativehelper/managed_jnienv.cc

        ${SOURCE_DIR}/common/PthreadExt.cpp
        ${SOURCE_DIR}/common/fake_dlfcn.c

        ${SOURCE_DIR}/libwechatbacktrace/FpUnwinder.cpp
        ${SOURCE_DIR}/libwechatbacktrace/Backtrace.cpp
)


# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${ANDROID_ABI}/libxhook.a
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${ANDROID_ABI}/libsemi_dlfcn.a

        PRIVATE ${log-lib}
        PRIVATE -Wl,--gc-sections
        PRIVATE -fvisibility=hidden
        android
        log)

target_include_directories(
        ${CMAKE_PROJECT_NAME}
        PUBLIC ${SOURCE_DIR}/common
        PUBLIC ${SOURCE_DIR}/libwechatbacktrace/include
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
)