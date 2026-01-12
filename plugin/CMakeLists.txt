cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)  # Enable module scanning

# Version is needed by JUCE.
project(AudioPlugin VERSION 0.1.0)

# Adding a directory with the library/application name as a subfolder of the
# include folder is a good practice. It helps avoid name clashes later on.
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/BBSynth")

# Adds a plugin target (that's basically what the Projucer does).
juce_add_plugin(
  ${PROJECT_NAME}
  COMPANY_NAME
  "chairbender" # change this
  IS_SYNTH
  TRUE # may change this
  NEEDS_MIDI_INPUT
  TRUE # may change this
  NEEDS_MIDI_OUTPUT
  FALSE # may change this
  PLUGIN_MANUFACTURER_CODE
  Cbdr # change this (for max compatibility, first letter must be uppercase, remaining lowercase)
  PLUGIN_CODE
  Bbsy # change this (for max compatibility, first letter must be uppercase, remaining lowercase)
  FORMATS
  Standalone
  VST3 # may change this
  PRODUCT_NAME
  "BBSynth" # change this
)

file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

# Sets the include directories of the plugin project.
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add the module file
target_sources(${PROJECT_NAME} PRIVATE
        FILE_SET CXX_MODULES FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/JuceImports.cppm")
# Disable strict warnings for the JUCE module wrapper
set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/JuceImports.cppm"
        PROPERTIES
        COMPILE_OPTIONS ""  # Clear all compile options
)

# Links to all necessary dependencies. The present ones are recommended by JUCE.
# If you use one of the additional modules, like the DSP module, you need to specify it here.
target_link_libraries_system(${PROJECT_NAME} PUBLIC juce::juce_audio_utils juce::juce_dsp)
target_link_libraries(
  ${PROJECT_NAME} PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
                         juce::juce_recommended_warning_flags
)

# These definitions are recommended by JUCE.
target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

# Enables strict C++ warnings and treats warnings as errors.
# This needs to be set up only for your projects, not 3rd party
file(GLOB_RECURSE SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)
set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # TODO: says stdlib arg unused during compilation - is this being passed wrong?
    target_compile_options(${PROJECT_NAME} PRIVATE
            -stdlib=libc++
            -fmodules
            -fprebuilt-module-path=${CMAKE_BINARY_DIR})
    target_link_options(${PROJECT_NAME} PRIVATE
            -stdlib=libc++
            -lc++abi)
    # Disable warnings for the module file
    set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/modules/JuceImports.cppm"
            PROPERTIES COMPILE_FLAGS "-w -stdlib=libc++")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
            -fmodules-ts)
    set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/modules/JuceImports.cppm"
            PROPERTIES COMPILE_FLAGS "-w")
elseif (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            /experimental:module
            /utf-8)
endif()

# In Visual Studio this command provides a nice grouping of source files in "filters".
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)
