cmake_minimum_required(VERSION 3.22.1)

project(VRTest)

add_library(native-lib SHARED native-lib.cpp)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv3)

add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

target_include_directories(native-lib PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue
        ${CMAKE_CURRENT_SOURCE_DIR}/External/glm)

target_link_libraries(native-lib
        native_app_glue
        ${android-lib}
        ${log-lib}
        ${egl-lib}
        ${gles-lib})