cmake_minimum_required(VERSION 3.22)
project(TremDeMedir VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the independently testable DSP core without JUCE/network:
option(TREM_DSP_TEST_ONLY "Build just the DSP unit test" OFF)
if(TREM_DSP_TEST_ONLY)
    enable_testing()
    add_executable(trem_dsp_test Source/MeterDSP.cpp tests/test_meter.cpp)
    target_include_directories(trem_dsp_test PRIVATE Source)
    add_test(NAME AudioMeterDSP COMMAND trem_dsp_test)
    return()
endif()

# JUCE is fetched at configure time on the user's machine.
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(TREM_FORMATS AU VST3 Standalone)
else()
    set(TREM_FORMATS VST3 Standalone)
endif()

juce_add_plugin(TremDeMedir
    COMPANY_NAME "Trem Audio"
    BUNDLE_ID "audio.trem.tremdemedir"
    PLUGIN_MANUFACTURER_CODE Tram
    PLUGIN_CODE Tmdr
    FORMATS ${TREM_FORMATS}
    PRODUCT_NAME "Trem de Medir"
    DESCRIPTION "Analisador de loudness, picos, fase e espectro"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

juce_generate_juce_header(TremDeMedir)
target_sources(TremDeMedir PRIVATE
    Source/MeterDSP.cpp Source/MeterDSP.h
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h)
target_link_libraries(TremDeMedir PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)
target_compile_definitions(TremDeMedir PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
