cmake_minimum_required(VERSION 3.22.1)
project("hoa")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PREBUILT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

# Imported prebuilt: hongengine C API wrapper
add_library(hongengine_c SHARED IMPORTED)
set_target_properties(hongengine_c PROPERTIES IMPORTED_LOCATION ${PREBUILT_DIR}/libhongengine_c.so)

# libhongengine.so — JNI bridge that links to the prebuilt ArkCompiler runtime
add_library(hongengine SHARED hongengine_jni.cpp)

target_include_directories(hongengine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party
)

target_link_libraries(hongengine
    hongengine_c
    android
    log
)
