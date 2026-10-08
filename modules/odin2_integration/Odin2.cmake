# Builds the Odin2 synth (modules/odin2, a git submodule) as part of SharedCode, so its
# OdinAudioProcessor can be hosted directly inside UpBeat (see source/Audio/OdinSynth.h).
#
# Odin's own CMakeLists.txt can't be add_subdirectory'd: it pulls in its own copy of JUCE
# and defines a complete plugin. Instead, its source and asset lists are read out of its
# CMake files (so they stay in sync when the submodule is updated) and compiled against
# our JUCE. Only Odin's libs/json and libs/tuning-library submodules are needed:
#   git -C modules/odin2 submodule update --init libs/json libs/tuning-library

set(ODIN2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/odin2")
set(ODIN2_INTEGRATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/odin2_integration")

foreach(requiredFile "${ODIN2_DIR}/CMakeLists.txt" "${ODIN2_DIR}/libs/json/include/nlohmann/json.hpp" "${ODIN2_DIR}/libs/tuning-library/include/Tunings.h")
    if(NOT EXISTS "${requiredFile}")
        message(FATAL_ERROR "Missing ${requiredFile}. Run: git submodule update --init modules/odin2 && git -C modules/odin2 submodule update --init libs/json libs/tuning-library")
    endif()
endforeach()

# ---- Sources: every "Source/....cpp" listed in Odin's CMakeLists.txt
file(READ "${ODIN2_DIR}/CMakeLists.txt" odinCMakeLists)
string(REGEX MATCHALL "\"Source/[^\"]+\\.cpp\"" OdinSourceFiles "${odinCMakeLists}")
list(TRANSFORM OdinSourceFiles REPLACE "\"" "")
list(TRANSFORM OdinSourceFiles PREPEND "${ODIN2_DIR}/")

# ---- Binary data: every quoted file in Odin's assets/CMakeLists.txt. Our own Assets target
# already owns the BinaryData namespace, so Odin's goes in OdinBinaryData, and
# JuceLibraryCode/JuceHeader.h aliases it back to BinaryData for Odin's code.
file(READ "${ODIN2_DIR}/assets/CMakeLists.txt" odinAssetsCMakeLists)
string(REGEX MATCHALL "\"[^\"]+\"" OdinAssetFiles "${odinAssetsCMakeLists}")
list(TRANSFORM OdinAssetFiles REPLACE "\"" "")
list(TRANSFORM OdinAssetFiles PREPEND "${ODIN2_DIR}/assets/")

juce_add_binary_data(Odin2Assets
    NAMESPACE OdinBinaryData
    HEADER_NAME OdinBinaryData.h
    SOURCES ${OdinAssetFiles})
set_target_properties(Odin2Assets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Odin includes GitCommitId.h, which its build normally generates.
set(ODIN2_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/odin2_generated")
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${ODIN2_DIR}"
    OUTPUT_VARIABLE ODIN2_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT ODIN2_GIT_HASH)
    set(ODIN2_GIT_HASH "deadbeef")
endif()
file(CONFIGURE OUTPUT "${ODIN2_GENERATED_DIR}/GitCommitId.h" CONTENT "#define GIT_COMMIT_ID \"${ODIN2_GIT_HASH}\"\n")

target_sources(SharedCode INTERFACE ${OdinSourceFiles})

# Odin's headers include "../JuceLibraryCode/JuceHeader.h" relative to whatever include
# directory they're found through, so the JuceLibraryCode directory itself goes on the path.
# Our code includes Odin's headers as <Source/...> through ODIN2_DIR, so Odin's
# PluginProcessor.h / PluginEditor.h never shadow ours.
# SYSTEM keeps Odin's header warnings out of our own translation units, except on MSVC: there
# it becomes /external:I, and returning from an external header restores the command-line
# warning level, undoing DisableWarnings.h below. source/Audio/OdinSynth.cpp silences
# Odin's headers with a pragma instead.
if(MSVC)
    set(ODIN2_INCLUDE_KIND "")
else()
    set(ODIN2_INCLUDE_KIND SYSTEM)
endif()
target_include_directories(SharedCode ${ODIN2_INCLUDE_KIND} INTERFACE
    "${ODIN2_DIR}"
    "${ODIN2_INTEGRATION_DIR}/JuceLibraryCode"
    "${ODIN2_GENERATED_DIR}"
    "${ODIN2_DIR}/libs/tuning-library"
    "${ODIN2_DIR}/libs/json/include")

target_link_libraries(SharedCode INTERFACE Odin2Assets)

# Odin's patch browser uses synchronous message boxes; this has to be set for the whole
# target because it changes which JUCE functions exist.
target_compile_definitions(SharedCode INTERFACE JUCE_MODAL_LOOPS_PERMITTED=1)

# Odin's headers use M_PI, which MSVC's <cmath> only defines with _USE_MATH_DEFINES. It must
# be set before <cmath> is first included, so it goes on every SharedCode translation unit.
target_compile_definitions(SharedCode INTERFACE _USE_MATH_DEFINES)

if(APPLE)
    set(ODIN2_PLATFORM_DEFINE ODIN_MAC=1)
elseif(UNIX)
    set(ODIN2_PLATFORM_DEFINE ODIN_LINUX=1)
else()
    set(ODIN2_PLATFORM_DEFINE ODIN_WIN=1)
endif()

set_source_files_properties(${OdinSourceFiles} PROPERTIES
    COMPILE_DEFINITIONS "${ODIN2_PLATFORM_DEFINE};CMAKE_SOURCE_DIRECTORY=\"${ODIN2_DIR}\"")

# Odin builds as its own plugin and defines createPluginFilter(); rename it so it doesn't
# collide with UpBeat's.
set_source_files_properties("${ODIN2_DIR}/Source/PluginProcessor.cpp" PROPERTIES
    COMPILE_DEFINITIONS "${ODIN2_PLATFORM_DEFINE};CMAKE_SOURCE_DIRECTORY=\"${ODIN2_DIR}\";createPluginFilter=createOdinPluginFilter")

# Third-party code: silence its warnings. On MSVC a forced-include of a
# `#pragma warning(push, 0)` header avoids the D9025 "overriding /W4" warning that /W0 causes.
if(MSVC)
    set_property(SOURCE ${OdinSourceFiles} APPEND PROPERTY COMPILE_OPTIONS "/FI${ODIN2_INTEGRATION_DIR}/DisableWarnings.h")
else()
    set_property(SOURCE ${OdinSourceFiles} APPEND PROPERTY COMPILE_OPTIONS "-w")
endif()
