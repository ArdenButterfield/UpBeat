//
// Created by Arden on 7/8/2026.
//

#include "ToleranceLabel.h"
#include "../GameState.h"
#include "BinaryData.h"

ToleranceLabel::ToleranceLabel (int category)
{
    setImage (imageForCategory (category), juce::RectanglePlacement::centred);
    setInterceptsMouseClicks (false, false);
}

juce::Image ToleranceLabel::imageForCategory (int category)
{
    struct ImageData
    {
        const char* data;
        int size;
    };
    // From best to worst, ending with a miss.
    static const std::array<ImageData, GameState::NUM_TOLERANCE_CATEGORIES + 1> images { {
        { BinaryData::hit_100_png, BinaryData::hit_100_pngSize },
        { BinaryData::hit_fresh_png, BinaryData::hit_fresh_pngSize },
        { BinaryData::hit_ok_png, BinaryData::hit_ok_pngSize },
        { BinaryData::hit_miss_png, BinaryData::hit_miss_pngSize },
    } };

    const auto& image = images[(size_t) juce::jlimit (0, (int) images.size() - 1, category)];
    return juce::ImageCache::getFromMemory (image.data, image.size);
}

bool ToleranceLabel::advance (double elapsedMs)
{
    ageMs += elapsedMs;

    if (ageMs <= visibleDurationMs)
    {
        setAlpha (1.0f);
    }
    else
    {
        auto fadeElapsed = ageMs - visibleDurationMs;
        setAlpha (juce::jlimit (0.0f, 1.0f, 1.0f - static_cast<float> (fadeElapsed / fadeDurationMs)));
    }

    return ageMs >= visibleDurationMs + fadeDurationMs;
}
