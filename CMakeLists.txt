cmake_minimum_required(VERSION 3.22)
project(PianoRoll VERSION 0.4.2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(PIANOROLL_CORE_ONLY "Build sequencing tests without JUCE" OFF)
enable_testing()
add_executable(pianoroll_tests tests/CoreTests.cpp)
target_include_directories(pianoroll_tests PRIVATE Source)
add_test(NAME sequencing COMMAND pianoroll_tests)
add_executable(pianoroll_editing_tests tests/EditingTests.cpp)
target_include_directories(pianoroll_editing_tests PRIVATE Source)
add_test(NAME editing COMMAND pianoroll_editing_tests)
if(PIANOROLL_CORE_ONLY)
  return()
endif()
set(JUCE_SOURCE_DIR "" CACHE PATH "Existing JUCE source directory (otherwise downloaded)")
if(JUCE_SOURCE_DIR)
  add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/juce")
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(PianoRoll
  COMPANY_NAME "Independent Audio"
  BUNDLE_ID "audio.independent.pianoroll"
  PLUGIN_MANUFACTURER_CODE Inau
  PLUGIN_CODE Pnrl
  PRODUCT_NAME "Piano Roll"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  FORMATS VST3 Standalone)
juce_generate_juce_header(PianoRoll)
target_sources(PianoRoll PRIVATE Source/Processor.cpp Source/Editor.cpp)
target_compile_definitions(PianoRoll PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(PianoRoll PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
option(PIANOROLL_LTO "Enable slower link-time optimization for release packaging" OFF)
if(PIANOROLL_LTO)
  target_link_libraries(PianoRoll PUBLIC juce::juce_recommended_lto_flags)
endif()
add_executable(pianoroll_integration_tests tests/IntegrationTests.cpp)
target_include_directories(pianoroll_integration_tests PRIVATE Source $<TARGET_PROPERTY:PianoRoll,INCLUDE_DIRECTORIES>)
target_compile_definitions(pianoroll_integration_tests PRIVATE $<TARGET_PROPERTY:PianoRoll,COMPILE_DEFINITIONS>)
target_link_libraries(pianoroll_integration_tests PRIVATE PianoRoll)
add_test(NAME processor_integration COMMAND pianoroll_integration_tests)
option(PIANOROLL_VERIFY_BINARY "Build an independent host for testing the compiled VST3" OFF)
if(PIANOROLL_VERIFY_BINARY)
  juce_add_console_app(PianoRollHostProbe PRODUCT_NAME "Piano Roll VST3 Host Probe")
  target_sources(PianoRollHostProbe PRIVATE tests/VST3HostProbe.cpp)
  target_compile_definitions(PianoRollHostProbe PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(PianoRollHostProbe PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
