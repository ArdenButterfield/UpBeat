//
// Created by Arden on 7/8/2026.
//

#include "BundledResources.h"
#include "BundledResourcesBinaryData.h"

juce::ZipFile& BundledResources::getZip()
{
    static juce::ZipFile zip (std::make_unique<juce::MemoryInputStream> (
        BundledResourcesBinaryData::BundledResources_zip, (size_t) BundledResourcesBinaryData::BundledResources_zipSize, false));
    return zip;
}

std::unique_ptr<juce::InputStream> BundledResources::createInputStream (const juce::String& relativePath)
{
    auto& zip = getZip();
    auto index = zip.getIndexOfFileName (relativePath);
    if (index < 0)
        return nullptr;

    return std::unique_ptr<juce::InputStream> (zip.createStreamForEntry (index));
}

juce::MemoryBlock BundledResources::loadFile (const juce::String& relativePath)
{
    juce::MemoryBlock block;
    if (auto stream = createInputStream (relativePath))
        stream->readIntoMemoryBlock (block);

    return block;
}

juce::StringArray BundledResources::listEntries (const juce::String& directoryRelativePath)
{
    auto prefix = directoryRelativePath.isEmpty()
                      ? juce::String()
                      : directoryRelativePath.trimCharactersAtEnd ("/") + "/";

    juce::StringArray result;
    auto& zip = getZip();
    for (int i = 0; i < zip.getNumEntries(); ++i)
        if (auto* entry = zip.getEntry (i))
            if (entry->filename.startsWith (prefix))
                result.add (entry->filename);

    return result;
}
