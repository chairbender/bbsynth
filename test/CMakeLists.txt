cmake_minimum_required(VERSION 3.22)

project(AudioPluginTest)

# Signals to CMake that we want to run tests from this directory.
enable_testing()

# Creates the test console application (DFT-focused to avoid building the full plugin).
set(SOURCE_FILES
        source/DFTTest.cpp
        ../plugin/include/BBSynth/MinBlepGenerator.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Sets the necessary include directories of googletest.
target_include_directories(${PROJECT_NAME} PRIVATE ${GOOGLETEST_SOURCE_DIR}/googletest/include)

# Thanks to the fact that we link against the gtest_main library, we don't have to write the main function ourselves.
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main)

# Enables strict C++ warnings and treats warnings as errors for our sources only.
set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")

# Include plugin headers path so tests can include BBSynth headers.
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../plugin/include)

# Adds googletest-specific CMake commands at our disposal.
include(GoogleTest)

# Add all tests defined with googletest to the CMake metadata
# so that these tests are run upon a call to ctest in the test
# projects' binary directory.
if(CMAKE_GENERATOR STREQUAL Xcode)
  # On macOS arm64, all binaries have to be signed before running.
  # In local development, the linker adds an ad-hoc placeholder signature.
  # In Xcode however, the ad-hoc signature is delayed until after the “Run Script” build phase,
  # so the POST_BUILD command added by gtest_discover_tests cannot run.
  # Thus, we need to delay test discovery until run time.
  # Source: https://discourse.cmake.org/t/googletest-crash-when-using-cmake-xcode-arm64/5766/8
  gtest_discover_tests(${PROJECT_NAME} DISCOVERY_MODE PRE_TEST)
else()
  gtest_discover_tests(${PROJECT_NAME})
endif()
