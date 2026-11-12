#ifndef UPBEAT_GUISTYLE_H
#define UPBEAT_GUISTYLE_H

#include "juce_gui_basics/juce_gui_basics.h"

// Text sizes and line weights shared across the GUI, so they can be tuned in one place.
namespace GuiStyle
{
    // Every font size is its design size scaled by this.
    constexpr float fontScale = 4.0f / 3.0f;

    constexpr float fontSize (float designSize) { return designSize * fontScale; }

    // Line weights, in pixels, from hairlines (grid and lane lines, outlines) up to emphasis
    // (the current bar's outline).
    constexpr int thinLine = 2;
    constexpr int mediumLine = 3;
    constexpr int thickLine = 4;

    // A horizontal line thickness px thick, centred on y. Lines of even thickness are crisp
    // when y falls on a pixel boundary, odd ones when it falls on a pixel's centre.
    inline void fillHorizontalLine (juce::Graphics& g, float y, float left, float right, int thickness)
    {
        auto top = std::floor (y - (float) thickness * 0.5f + 0.5f);
        g.fillRect (juce::Rectangle<float> (left, top, right - left, (float) thickness));
    }

    // A vertical line thickness px thick, centred on x (see fillHorizontalLine).
    inline void fillVerticalLine (juce::Graphics& g, float x, float top, float bottom, int thickness)
    {
        auto left = std::floor (x - (float) thickness * 0.5f + 0.5f);
        g.fillRect (juce::Rectangle<float> (left, top, (float) thickness, bottom - top));
    }

    // JUCE's own widgets (buttons, labels, slider text boxes) drawn with the game's larger
    // text and heavier outlines.
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
        {
            return juce::FontOptions (juce::jmin (fontSize (16.0f), (float) buttonHeight * 0.6f));
        }

        juce::Font getLabelFont (juce::Label& label) override
        {
            auto font = label.getFont();
            return font.withHeight (fontSize (font.getHeight()));
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool isHighlighted, bool isDown) override
        {
            // As LookAndFeel_V4 draws it, with a heavier outline.
            auto cornerSize = 6.0f;
            auto bounds = button.getLocalBounds().toFloat().reduced ((float) thinLine * 0.5f);

            auto colour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                              .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);
            if (isDown || isHighlighted)
                colour = colour.contrasting (isDown ? 0.2f : 0.05f);

            g.setColour (colour);
            g.fillRoundedRectangle (bounds, cornerSize);
            g.setColour (button.findColour (juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle (bounds, cornerSize, (float) thinLine);
        }
    };
}

#endif //UPBEAT_GUISTYLE_H
