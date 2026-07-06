cmake_minimum_required(VERSION 3.22)

project(Ondaire VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Fetch JUCE if not provided by the environment (set JUCE_SOURCE_DIR to use a local copy).
if(DEFINED JUCE_SOURCE_DIR)
    add_subdirectory(${JUCE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Ondaire
    COMPANY_NAME              "Ondaire"
    PRODUCT_NAME              "Ondaire"
    BUNDLE_ID                 "com.ondaire.ondaire"
    PLUGIN_MANUFACTURER_CODE  Ondr
    PLUGIN_CODE               Onda
    FORMATS                   VST3 Standalone
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Instrument Synth
)

target_sources(Ondaire PRIVATE
    Source/OndaireEngine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

juce_add_binary_data(OndaireAssets
    HEADER_NAME OndaireAssets.h
    NAMESPACE OndaireAssets
    SOURCES Assets/background.png
)

target_compile_definitions(Ondaire PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Ondaire
    PRIVATE
        OndaireAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Headless smoke test that renders every factory preset through the engine.
option(ONDAIRE_BUILD_TESTS "Build the Ondaire engine smoke test" ON)
if(ONDAIRE_BUILD_TESTS)
    juce_add_console_app(OndaireEngineTest PRODUCT_NAME "OndaireEngineTest")
    target_sources(OndaireEngineTest PRIVATE
        Source/Tests/EngineSmokeTest.cpp
        Source/OndaireEngine.cpp
    )
    target_compile_definitions(OndaireEngineTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    target_link_libraries(OndaireEngineTest PRIVATE
        juce::juce_audio_basics
        juce::juce_recommended_config_flags
    )
    enable_testing()
    add_test(NAME EngineSmokeTest COMMAND OndaireEngineTest)
endif()
