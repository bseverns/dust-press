cmake_minimum_required(VERSION 3.16)

include(FetchContent)

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 7.0.12
)

FetchContent_MakeAvailable(juce)

juce_add_plugin(dustpress_plugin
  COMPANY_NAME "dustpress"
  PLUGIN_MANUFACTURER_CODE DSPR
  PLUGIN_CODE DuPr
  FORMATS VST3 AU
  PRODUCT_NAME "Dust Press"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
  SOURCES
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
)

juce_generate_juce_header(dustpress_plugin)

target_link_libraries(dustpress_plugin PRIVATE juce::juce_audio_utils dustpress_native)

target_include_directories(dustpress_plugin PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)
