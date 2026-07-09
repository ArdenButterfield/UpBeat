//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_BUNDLEDRESOURCES_H
#define UPBEAT_BUNDLEDRESOURCES_H

#include "juce_core/juce_core.h"
#include <memory>

// Reads files out of the BundledResources/ directory that's embedded into the binary
// as a zip (see CMakeLists.txt) so its folder structure survives JUCE's BinaryData
// flattening. Paths are relative to BundledResources/, e.g.
// "default_charts/midi/hello.mid".
class BundledResources
{
public:
    static std::unique_ptr<juce::InputStream> createInputStream (const juce::String& relativePath);
    static juce::MemoryBlock loadFile (const juce::String& relativePath);

    // Returns the paths of every entry nested under directoryRelativePath (recursive).
    // Pass an empty string to list the entire archive.
    static juce::StringArray listEntries (const juce::String& directoryRelativePath = {});

private:
    static juce::ZipFile& getZip();
};

#endif //UPBEAT_BUNDLEDRESOURCES_H
