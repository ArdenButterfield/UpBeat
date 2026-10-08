// Stand-in for the JuceHeader.h that Odin2's own build generates with
// juce_generate_juce_header(). Odin's sources include it as
// "../JuceLibraryCode/JuceHeader.h", which resolves here because this directory is on the
// include path (see ../Odin2.cmake).

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

// Odin's binary data is built into its own namespace so it doesn't collide with UpBeat's
// BinaryData; Odin's code refers to it as BinaryData.
#include <OdinBinaryData.h>
namespace BinaryData = OdinBinaryData;

using namespace juce;
