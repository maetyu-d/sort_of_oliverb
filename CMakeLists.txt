cmake_minimum_required(VERSION 3.22)

project(MiVerb VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
  add_subdirectory(JUCE)
else()
  find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_plugin(MiVerb
  COMPANY_NAME "MutablePort"
  BUNDLE_ID "com.mutableport.miverb"
  PLUGIN_MANUFACTURER_CODE MvRb
  PLUGIN_CODE MiVb
  FORMATS AU VST3 Standalone
  PRODUCT_NAME "MiVerb"
)

juce_generate_juce_header(MiVerb)

target_sources(MiVerb
  PRIVATE
    Source/CloudsFxEngine.h
    Source/CloudsReverb.h
    Source/OliverbReverb.h
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
)

target_compile_definitions(MiVerb
  PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_FORCE_USE_LEGACY_PARAM_IDS=1
)

target_link_libraries(MiVerb
  PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
