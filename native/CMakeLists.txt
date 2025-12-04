cmake_minimum_required(VERSION 3.16)
project(dustpress_native VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(dustpress_native STATIC
  ../src/AirEQ.cpp
  ../src/CurveBank.cpp
  ../src/EnvelopeFollower.cpp
  ../src/LimiterLookahead.cpp
  ../src/ParamSmoother.cpp
  ../src/SoftSaturation.cpp
  ../src/TiltEQ.cpp
  src/NativeDustPress.cpp
  src/PluginState.cpp
  src/PresetLoader.cpp
  src/WavFile.cpp
)

target_include_directories(dustpress_native
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_features(dustpress_native PUBLIC cxx_std_17)

add_executable(dustpress_cli src/main.cpp)
target_link_libraries(dustpress_cli PRIVATE dustpress_native)

target_include_directories(dustpress_cli PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

option(DUSTPRESS_BUILD_JUCE_PLUGIN "Build the JUCE VST3/AU preview plugin" OFF)
if(DUSTPRESS_BUILD_JUCE_PLUGIN)
  add_subdirectory(juce)
endif()

add_executable(dustpress_probe src/probe.cpp)
target_link_libraries(dustpress_probe PRIVATE dustpress_native)
target_include_directories(dustpress_probe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

option(DUSTPRESS_BUILD_PLUGIN "Build the JUCE/VST3 wrapper around NativeDustPress" OFF)

# Convenience: if you're chasing the plugin and haven't declared how to get JUCE,
# auto-opt into the FetchContent path so newcomers don't smash into missing deps.
option(DUSTPRESS_AUTO_FETCH_JUCE_WHEN_PLUGIN "Auto-enable JUCE fetching when building the plugin unless you explicitly set DUSTPRESS_FETCH_JUCE" ON)

if(DUSTPRESS_BUILD_PLUGIN AND DUSTPRESS_AUTO_FETCH_JUCE_WHEN_PLUGIN)
  if(NOT DEFINED DUSTPRESS_FETCH_JUCE)
    set(DUSTPRESS_FETCH_JUCE ON CACHE BOOL "Automatically fetch JUCE when building the plugin" FORCE)
  endif()
endif()

if(DUSTPRESS_BUILD_PLUGIN)
  add_subdirectory(plugin)
endif()

