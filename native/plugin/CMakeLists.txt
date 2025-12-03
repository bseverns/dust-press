# JUCE plugin wrapper for the NativeDustPress DSP

option(DUSTPRESS_FETCH_JUCE "Automatically fetch JUCE (plus the VST3 SDK) for plugin builds" ON)
option(DUSTPRESS_FETCH_VST3_SDK "Allow the JUCE build to pull down the Steinberg VST3 SDK" ON)
set(DUSTPRESS_JUCE_GIT_TAG "7.0.12" CACHE STRING "JUCE tag/branch to fetch when auto-downloading")

set(DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR "${PROJECT_SOURCE_DIR}/.juce-kit" CACHE PATH "Optional JUCE install dir to reuse instead of fetching")
set(DUSTPRESS_LOCAL_JUCE_BUILD_DIR "${PROJECT_SOURCE_DIR}/.juce-build" CACHE PATH "Legacy pre-configured JUCE build tree to reuse instead of fetching")

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

  # Use a repo-local JUCE install dir by default if one already exists.
  if(NOT JUCE_DIR AND EXISTS "${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}/lib/cmake/JUCE-${DUSTPRESS_JUCE_GIT_TAG}/JUCEConfig.cmake")
    set(JUCE_DIR "${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}/lib/cmake/JUCE-${DUSTPRESS_JUCE_GIT_TAG}" CACHE PATH "JUCE package dir (auto-populated when present)" FORCE)
  elseif(NOT JUCE_DIR AND EXISTS "${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}/JUCEConfig.cmake")
    # Legacy hand-configured build tree support.
    set(JUCE_DIR "${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}" CACHE PATH "JUCE build dir (auto-populated when present)" FORCE)
  endif()

  # If the user points JUCE_DIR at a raw build tree, CMake will happily pick it
  # up and then promptly explode when JUCEConfig tries to include helper files
  # that only exist in an *installed* JUCE package. Preflight the helpers before
  # calling find_package so the error message is loud and actionable instead of
  # a mysterious "Unknown CMake command juce_add_module".
  if(JUCE_DIR)
    if(NOT EXISTS "${JUCE_DIR}/JUCEConfig.cmake")
      message(FATAL_ERROR
        "JUCE_DIR='${JUCE_DIR}' does not contain JUCEConfig.cmake. Point it at an installed JUCE prefix (e.g. <juce>/kit/lib/cmake/JUCE-7.0.12) or run tools/bootstrap_juce.sh to drop one into native/.juce-kit.")
    endif()

    set(_dustpress_missing_helpers)
    foreach(_dustpress_helper IN ITEMS LV2_HELPER.cmake JUCEModuleSupport.cmake JUCEUtils.cmake)
      if(NOT EXISTS "${JUCE_DIR}/${_dustpress_helper}")
        list(APPEND _dustpress_missing_helpers "${_dustpress_helper}")
      endif()
    endforeach()

    if(_dustpress_missing_helpers)
      string(REPLACE ";" "\n- " _dustpress_helper_list "${_dustpress_missing_helpers}")
      message(FATAL_ERROR
        "JUCE_DIR='${JUCE_DIR}' was set, but JUCE helper files are missing:\n- ${_dustpress_helper_list}\n"
        "This usually means you pointed JUCE_DIR/CMAKE_PREFIX_PATH at a JUCE build tree that was never installed."
        " Reconfigure JUCE with an install prefix and run 'cmake --install', or just run tools/bootstrap_juce.sh to drop a JUCE kit into native/.juce-kit and try again.")
    endif()
  endif()

  find_package(JUCE CONFIG QUIET)

  if(NOT JUCE_FOUND)
    message(FATAL_ERROR "JUCE not found. Either set DUSTPRESS_FETCH_JUCE=ON (default) so CMake downloads it, or point JUCE_DIR/CMAKE_PREFIX_PATH at a JUCE build directory that contains JUCEConfig.cmake (e.g. <juce>/build). Then re-run CMake with DUSTPRESS_BUILD_PLUGIN=ON.")
  endif()

  # Sanity-check the JUCE package for the helper files that JUCEConfig pulls in
  # when it was discovered via CMAKE_PREFIX_PATH instead of a direct JUCE_DIR
  # hint. If CMake found a raw build directory, we still want to emit a clearer
  # failure than "Unknown CMake command juce_add_module".
  set(_dustpress_juce_config_dir "${JUCE_DIR}")
  if(NOT _dustpress_juce_config_dir AND DEFINED JUCE_CONFIG)
    get_filename_component(_dustpress_juce_config_dir "${JUCE_CONFIG}" DIRECTORY)
  endif()

  if(_dustpress_juce_config_dir)
    set(_dustpress_missing_helpers)
    foreach(_dustpress_helper IN ITEMS LV2_HELPER.cmake JUCEModuleSupport.cmake JUCEUtils.cmake)
      if(NOT EXISTS "${_dustpress_juce_config_dir}/${_dustpress_helper}")
        list(APPEND _dustpress_missing_helpers "${_dustpress_helper}")
      endif()
    endforeach()

    if(_dustpress_missing_helpers)
      string(REPLACE ";" "\n- " _dustpress_helper_list "${_dustpress_missing_helpers}")
      message(FATAL_ERROR
        "JUCEConfig.cmake was found at '${_dustpress_juce_config_dir}', but the following helper files are missing:\n- ${_dustpress_helper_list}\n"
        "This usually means you pointed CMAKE_PREFIX_PATH/JUCE_DIR at a JUCE build tree that was never installed."
        " Reconfigure JUCE with an install prefix and run 'cmake --install', or just run tools/bootstrap_juce.sh to drop a JUCE kit into native/.juce-kit and try again.")
    endif()
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
