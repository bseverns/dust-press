# JUCE plugin wrapper for the NativeDustPress DSP

option(DUSTPRESS_FETCH_JUCE "Automatically fetch JUCE (plus the VST3 SDK) for plugin builds" ON)
option(DUSTPRESS_FETCH_VST3_SDK "Allow the JUCE build to pull down the Steinberg VST3 SDK" ON)
option(DUSTPRESS_INSTALL_FETCHED_JUCE "Install auto-fetched JUCE into a reusable native/.juce-kit" ON)

set(DUSTPRESS_JUCE_GIT_TAG "8.0.11" CACHE STRING "JUCE tag/branch to fetch when auto-downloading")

set(DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR "${PROJECT_SOURCE_DIR}/.juce-kit" CACHE PATH "Optional JUCE install dir to reuse instead of fetching")
set(DUSTPRESS_LOCAL_JUCE_BUILD_DIR "${PROJECT_SOURCE_DIR}/.juce-build" CACHE PATH "Legacy pre-configured JUCE build tree to reuse instead of fetching")
set(_dustpress_required_juce_helpers LV2_HELPER.cmake JUCEModuleSupport.cmake JUCEUtils.cmake)

if(DUSTPRESS_FETCH_JUCE)
  include(FetchContent)
  set(JUCE_ENABLE_MODULE_SOURCE_GROUPS OFF CACHE BOOL "" FORCE)
  set(JUCE_FETCH_VST3_SDK ${DUSTPRESS_FETCH_VST3_SDK} CACHE BOOL "" FORCE)

  FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${DUSTPRESS_JUCE_GIT_TAG}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}
      -DJUCE_USE_X11=0
      -DJUCE_USE_XRANDR=0
      -DJUCE_USE_XINERAMA=0
      -DCMAKE_CXX_FLAGS=-DJUCE_USE_XRANDR=0\ -DJUCE_USE_XINERAMA=0
      -DJUCE_USE_ALSA=0
      # juceaide (the codegen tool JUCE bakes into the build) only lands in the
      # extras target set. We install JUCE as part of the build to prime a
      # reusable kit under native/.juce-kit, and the install step shells out to
      # `cmake --build ... --target juceaide` before running `cmake --install`
      # so JUCE’s helper CMake modules actually make it into that prefix. Make
      # sure the extras are on so that target exists for Ninja to build.
      -DJUCE_BUILD_EXTRAS=ON
  )

  FetchContent_MakeAvailable(juce)

  # Force JUCE’s GUI modules to skip optional X11 multi-head helpers so the
  # juceaide build doesn’t rely on headers that aren’t present in minimal CI
  # images.
  set(_dustpress_gui_basics_header "${juce_SOURCE_DIR}/modules/juce_gui_basics/juce_gui_basics.h")
  if(EXISTS "${_dustpress_gui_basics_header}")
    file(READ "${_dustpress_gui_basics_header}" _dustpress_gui_basics_contents)
    set(_dustpress_gui_basics_patched "${_dustpress_gui_basics_contents}")

    string(REPLACE
      "#ifndef JUCE_USE_XRANDR\n #define JUCE_USE_XRANDR 1"
      "#ifndef JUCE_USE_XRANDR\n #define JUCE_USE_XRANDR 0"
      _dustpress_gui_basics_patched
      "${_dustpress_gui_basics_patched}"
    )

    string(REPLACE
      "#ifndef JUCE_USE_XINERAMA\n #define JUCE_USE_XINERAMA 1"
      "#ifndef JUCE_USE_XINERAMA\n #define JUCE_USE_XINERAMA 0"
      _dustpress_gui_basics_patched
      "${_dustpress_gui_basics_patched}"
    )

    if(NOT _dustpress_gui_basics_patched STREQUAL _dustpress_gui_basics_contents)
      file(WRITE "${_dustpress_gui_basics_header}" "${_dustpress_gui_basics_patched}")
    endif()
  endif()

  # On newer AppleClang/libc++ (Xcode 15+) the CoreAudio StrideIterator used in
  # juce_CoreAudio_mac.cpp is missing the bits that std::iterator_traits expects.
  # That shows up as:
  #
  #   no type named 'value_type' in 'std::iterator_traits<
  #     juce::CoreAudioClasses::CoreAudioInternal::StrideIterator<const float *>>'
  #
  # To keep using JUCE 8.0.11 cleanly on Intel CI, bolt on an iterator_traits
  # specialization for that internal iterator type.
  if(APPLE)
    set(_dustpress_coreaudio_mac
      "${juce_SOURCE_DIR}/modules/juce_audio_devices/native/juce_CoreAudio_mac.cpp"
    )

    if(EXISTS "${_dustpress_coreaudio_mac}")
      file(READ "${_dustpress_coreaudio_mac}" _dustpress_coreaudio_contents)

      # Only inject once; if the marker is already present, skip.
      if(NOT _dustpress_coreaudio_contents MATCHES "DustPress StrideIterator iterator_traits shim")
        string(APPEND _dustpress_coreaudio_contents [[

// DustPress StrideIterator iterator_traits shim.
// Makes juce::CoreAudioClasses::CoreAudioInternal::StrideIterator work with
// std::iterator_traits on newer AppleClang/libc++.
#if defined(__APPLE__)
namespace std
{
    template <typename Iterator>
    struct iterator_traits<
        juce::CoreAudioClasses::CoreAudioInternal::StrideIterator<Iterator>>
    {
        using value_type        = typename iterator_traits<Iterator>::value_type;
        using difference_type   = typename iterator_traits<Iterator>::difference_type;
        using pointer           = typename iterator_traits<Iterator>::pointer;
        using reference         = typename iterator_traits<Iterator>::reference;
        using iterator_category = random_access_iterator_tag;
    };
}
#endif // __APPLE__

]])

        file(WRITE "${_dustpress_coreaudio_mac}" "${_dustpress_coreaudio_contents}")
      endif()
    endif()
  endif()

  if(DUSTPRESS_INSTALL_FETCHED_JUCE)
    FetchContent_GetProperties(juce)

    if(NOT juce_BINARY_DIR)
      message(FATAL_ERROR "Expected juce_BINARY_DIR after FetchContent population; cannot install JUCE kit.")
    endif()

    set(_dustpress_juce_install_config "${CMAKE_BUILD_TYPE}")
    if(NOT _dustpress_juce_install_config AND CMAKE_CONFIGURATION_TYPES)
      list(GET CMAKE_CONFIGURATION_TYPES 0 _dustpress_juce_install_config)
    endif()
    if(NOT _dustpress_juce_install_config)
      set(_dustpress_juce_install_config "Release")
    endif()

    # Always push the JUCE install into our repo-local kit instead of the
    # default /usr/local prefix so CI jobs without sudo do not explode mid-build.
    file(MAKE_DIRECTORY "${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}")

    # Some environments drop JUCE into our build tree without the extras already
    # wired up (and therefore without a juceaide target for the install step).
    # If we can’t see juceaide yet, explicitly pull in the extras CMakeLists to
    # stitch that target into our main Ninja graph before we try to build it.
    if(NOT TARGET juceaide)
      add_compile_definitions(JUCE_USE_X11=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0)
      add_subdirectory("${juce_SOURCE_DIR}/extras/Build" "${juce_BINARY_DIR}/extras")
    endif()

    # JUCE’s build glues juceaide into the configure step rather than exposing a
    # durable build target. Drive the install directly; juceaide will have been
    # produced while configuring the extras subdir above.
    set(_dustpress_juceaide_bin
      "${juce_BINARY_DIR}/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide"
    )

    # JUCE lives inside the top-level Ninja graph; its build.ninja does not sit
    # under juce_BINARY_DIR. Drive the build/install from the main binary dir
    # so CMake can find the generator files it wrote for the fetch content.
    add_custom_target(dustpress_install_fetched_juce
      COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}"
              --config "${_dustpress_juce_install_config}"
              --prefix "${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}"
      BYPRODUCTS
        "${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}/lib/cmake/JUCE-${DUSTPRESS_JUCE_GIT_TAG}/JUCEConfig.cmake"
        "${_dustpress_juceaide_bin}"
      COMMENT "Installing fetched JUCE into ${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR} (${_dustpress_juce_install_config})"
      VERBATIM
    )
  endif()
else()
  if(DUSTPRESS_FETCH_VST3_SDK)
    set(JUCE_FETCH_VST3_SDK ON CACHE BOOL "" FORCE)
  endif()

  # Use a repo-local JUCE install dir by default if one already exists.
  if(NOT JUCE_DIR AND EXISTS "${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}/lib/cmake/JUCE-${DUSTPRESS_JUCE_GIT_TAG}/JUCEConfig.cmake")
    set(JUCE_DIR "${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR}/lib/cmake/JUCE-${DUSTPRESS_JUCE_GIT_TAG}" CACHE PATH "JUCE package dir (auto-populated when present)" FORCE)
  elseif(NOT JUCE_DIR AND EXISTS "${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}/JUCEConfig.cmake")
    set(_dustpress_build_helpers_present ON)
    foreach(_dustpress_helper IN LISTS _dustpress_required_juce_helpers)
      if(NOT EXISTS "${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}/${_dustpress_helper}")
        set(_dustpress_build_helpers_present OFF)
        break()
      endif()
    endforeach()

    if(_dustpress_build_helpers_present)
      # Legacy hand-configured build tree support.
      set(JUCE_DIR "${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}" CACHE PATH "JUCE build dir (auto-populated when present)" FORCE)
    else()
      message(STATUS
        "Skipping legacy JUCE build directory at ${DUSTPRESS_LOCAL_JUCE_BUILD_DIR}; helper files are missing. "
        "Run tools/bootstrap_juce.sh or install JUCE to a prefix and point JUCE_DIR/CMAKE_PREFIX_PATH at it instead."
      )
    endif()
  endif()

  # If the user points JUCE_DIR at a raw build tree, CMake will happily pick it
  # up and then promptly explode when JUCEConfig tries to include helper files
  # that only exist in an *installed* JUCE package. Preflight the helpers before
  # calling find_package so the error message is loud and actionable instead of
  # a mysterious "Unknown CMake command juce_add_module".
  if(JUCE_DIR)
    if(NOT EXISTS "${JUCE_DIR}/JUCEConfig.cmake")
      message(FATAL_ERROR
        "JUCE_DIR='${JUCE_DIR}' does not contain JUCEConfig.cmake. "
        "Point it at an installed JUCE prefix (e.g. <juce>/kit/lib/cmake/JUCE-8.0.11) "
        "or run tools/bootstrap_juce.sh to drop one into native/.juce-kit."
      )
    endif()

    set(_dustpress_missing_helpers)
    foreach(_dustpress_helper IN LISTS _dustpress_required_juce_helpers)
      if(NOT EXISTS "${JUCE_DIR}/${_dustpress_helper}")
        list(APPEND _dustpress_missing_helpers "${JUCE_DIR}/${_dustpress_helper}")
      endif()
    endforeach()

    if(_dustpress_missing_helpers)
      string(REPLACE ";" "\n- " _dustpress_helper_list "${_dustpress_missing_helpers}")
      message(FATAL_ERROR
        "JUCE_DIR='${JUCE_DIR}' was set, but JUCE helper files are missing:\n- ${_dustpress_helper_list}\n"
        "That path points at a raw JUCE build tree, not an installed kit with the helper CMake modules. "
        "Reconfigure JUCE with an install prefix and run 'cmake --install', or let this project do it for you with "
        "DUSTPRESS_FETCH_JUCE=ON (default) / tools/bootstrap_juce.sh so ${DUSTPRESS_LOCAL_JUCE_PACKAGE_DIR} gains a full JUCE install. "
        "Then retry with JUCE_DIR or CMAKE_PREFIX_PATH aimed at that install instead of the build dir."
      )
    endif()
  endif()

  find_package(JUCE CONFIG QUIET)

  if(NOT JUCE_FOUND)
    message(FATAL_ERROR
      "JUCE not found. Either set DUSTPRESS_FETCH_JUCE=ON (default) so CMake downloads it, or point "
      "JUCE_DIR/CMAKE_PREFIX_PATH at a JUCE build directory that contains JUCEConfig.cmake (e.g. <juce>/build). "
      "Then re-run CMake with DUSTPRESS_BUILD_PLUGIN=ON."
    )
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
    foreach(_dustpress_helper IN LISTS _dustpress_required_juce_helpers)
      if(NOT EXISTS "${_dustpress_juce_config_dir}/${_dustpress_helper}")
        list(APPEND _dustpress_missing_helpers "${_dustpress_helper}")
      endif()
    endforeach()

    if(_dustpress_missing_helpers)
      string(REPLACE ";" "\n- " _dustpress_helper_list "${_dustpress_missing_helpers}")
      message(FATAL_ERROR
        "JUCEConfig.cmake was found at '${_dustpress_juce_config_dir}', but the following helper files are missing:\n- ${_dustpress_helper_list}\n"
        "This usually means you pointed CMAKE_PREFIX_PATH/JUCE_DIR at a JUCE build tree that was never installed. "
        "Reconfigure JUCE with an install prefix and run 'cmake --install', or just run tools/bootstrap_juce.sh to drop a JUCE kit into "
        "native/.juce-kit and try again."
      )
    endif()
  endif()
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------

set(DUSTPRESS_PLUGIN_TARGET DustPressPlugin)

juce_add_plugin(${DUSTPRESS_PLUGIN_TARGET}
  COMPANY_NAME              "Dust Press"
  VERSION                   ${PROJECT_VERSION}
  BUNDLE_ID                 "com.dustpress.DustPressPlugin"
  IS_SYNTH                  FALSE
  NEEDS_MIDI_INPUT          FALSE
  NEEDS_MIDI_OUTPUT         FALSE
  IS_MIDI_EFFECT            FALSE
  PLUGIN_MANUFACTURER_CODE  Dspr
  PLUGIN_CODE               Dp01
  FORMATS                   VST3
  PRODUCT_NAME              "DustPress"
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

if(DUSTPRESS_FETCH_JUCE AND DUSTPRESS_INSTALL_FETCHED_JUCE)
  add_dependencies(${DUSTPRESS_PLUGIN_TARGET} dustpress_install_fetched_juce)
endif()
