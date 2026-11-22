cmake_minimum_required(VERSION 3.22)
project(MegaPlugin VERSION 1.0.0)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(MegaPlugin
    COMPANY_NAME "Art Anzhelii"
    PLUGIN_MANUFACTURER_CODE Anzh
    PLUGIN_CODE Mega
    FORMATS VST3
    PRODUCT_NAME "Super Krutoy Mega Plagin"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(MegaPlugin)
target_sources(MegaPlugin PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(MegaPlugin PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
if(MSVC)
  target_compile_options(MegaPlugin PRIVATE /utf-8)
endif()
target_link_libraries(MegaPlugin PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
