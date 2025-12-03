# JUCE plugin wrapper for the NativeDustPress DSP

find_package(JUCE CONFIG REQUIRED)

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
