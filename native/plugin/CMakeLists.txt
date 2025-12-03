# JUCE plugin wrapper for the NativeDustPress DSP

option(DUSTPRESS_FETCH_JUCE "Automatically fetch JUCE (plus the VST3 SDK) for plugin builds" ON)
option(DUSTPRESS_FETCH_VST3_SDK "Allow the JUCE build to pull down the Steinberg VST3 SDK" ON)
set(DUSTPRESS_JUCE_GIT_TAG "7.0.12" CACHE STRING "JUCE tag/branch to fetch when auto-downloading")

set(DUSTPRESS_LOCAL_JUCE_BUILD_DIR "${PROJECT_SOURCE_DIR}/.juce-build" CACHE PATH "Optional pre-configured JUCE build tree to reuse instead of fetching")

if(DUSTPRESS_FETCH_JUCE)
  include(FetchContent)
  set(JUCE_ENABLE_MODULE_SOURCE_GROUPS OFF CACHE BOOL "" FORCE)
  set(JUCE_FETCH_VST3_SDK ${DUSTPRESS_FETCH_VST3_SDK} CACHE BOOL "" FORCE)

  FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${DUSTPRESS_JUCE_GIT_TAG}
  )

  FetchContent_MakeAvailable(juce)
else()
  if(DUSTPRESS_FETCH_VST3_SDK)
    set(JUCE_FETCH_VST3_SDK ON CACHE BOOL "" FORCE)
  endif()

  # Use a repo-local JUCE build dir by default if one already exists.
  if(NOT JUCE_DIR AND EXISTS "${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}/JUCEConfig.cmake")
    set(JUCE_DIR "${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}" CACHE PATH "JUCE build dir (auto-populated when present)" FORCE)
  endif()

  find_package(JUCE CONFIG QUIET)

  if(NOT JUCE_FOUND)
    message(FATAL_ERROR "JUCE not found. Either set DUSTPRESS_FETCH_JUCE=ON (default) so CMake downloads it, or point JUCE_DIR/CMAKE_PREFIX_PATH at a JUCE build directory that contains JUCEConfig.cmake (e.g. <juce>/build). Then re-run CMake with DUSTPRESS_BUILD_PLUGIN=ON.")
  endif()
endif()

set(DUSTPRESS_PLUGIN_TARGET DustPressPlugin)

juce_add_plugin(${DUSTPRESS_PLUGIN_TARGET}
  COMPANY_NAME "Dust Press"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  PLUGIN_MANUFACTURER_CODE Dspr
  PLUGIN_CODE Dp01
  FORMATS VST3
  PRODUCT_NAME "DustPress"
)

juce_generate_juce_header(${DUSTPRESS_PLUGIN_TARGET})

set(DUSTPRESS_PLUGIN_SOURCES
  DustPressAudioProcessor.cpp
  DustPressAudioProcessor.h
  DustPressAudioProcessorEditor.cpp
  DustPressAudioProcessorEditor.h
)

target_sources(${DUSTPRESS_PLUGIN_TARGET}
  PRIVATE
    ${DUSTPRESS_PLUGIN_SOURCES}
)

target_link_libraries(${DUSTPRESS_PLUGIN_TARGET}
  PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    dustpress_native
)

target_compile_definitions(${DUSTPRESS_PLUGIN_TARGET}
  PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
