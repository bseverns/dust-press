cmake_minimum_required(VERSION 3.16)
project(dustpress_native VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# macOS builds can default to multi-arch (x86_64 + arm64) when using certain
# generators, which is a fast path to "my M1 build won’t link" pain. If you're
# on Apple Silicon and didn't explicitly ask for a fat binary, lock the build
# to arm64 so the toolchain stays on the rails.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
  # Prefer the host CPU when Rosetta is involved: the system processor can
  # report x86_64 even when the machine is arm64.
  set(_dustpress_host_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
  if(NOT _dustpress_host_arch)
    set(_dustpress_host_arch "${CMAKE_SYSTEM_PROCESSOR}")
  endif()
  if(NOT _dustpress_host_arch AND CMAKE_OSX_ARCHITECTURES)
    set(_dustpress_host_arch "${CMAKE_OSX_ARCHITECTURES}")
  endif()
  if(_dustpress_host_arch MATCHES "^(arm64|arm64e|aarch64)$")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures to build" FORCE)
  else()
    # Rosetta can report x86_64 even on Apple Silicon; confirm via sysctl.
    execute_process(
      COMMAND sysctl -n hw.optional.arm64
      OUTPUT_VARIABLE _dustpress_hw_arm64
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(_dustpress_hw_arm64 STREQUAL "1")
      set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures to build" FORCE)
    endif()
    unset(_dustpress_hw_arm64)
  endif()
  unset(_dustpress_host_arch)
endif()

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
