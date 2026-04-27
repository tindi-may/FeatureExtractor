#pragma once
#include <JuceHeader.h>

class CircleIconButton : public juce::Button
{
public:
    CircleIconButton(const juce::String& name, const char* svgData, int svgSize)
        : juce::Button(name)
    {
        icon = juce::Drawable::createFromImageData(svgData, svgSize);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        bool isActive = getToggleState(); 

        if (isActive) {
            g.setColour(findColour(juce::TextButton::buttonOnColourId));
        }
        else {
            g.setColour(juce::Colours::white.withAlpha(0.9f));
        }

        g.fillEllipse(bounds);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.fillEllipse(bounds);
        }

        if (icon != nullptr) {
            auto iconBounds = bounds.reduced(bounds.getWidth() * 0.25f);
            auto iconColor = isActive ? juce::Colours::white : juce::Colours::black;

            icon->replaceColour(juce::Colours::black, iconColor);
            icon->replaceColour(juce::Colours::white, iconColor);

            icon->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
        }
    }

private:
    std::unique_ptr<juce::Drawable> icon;
};

class SectionPanel : public juce::Component
{
public:
    SectionPanel(juce::String name, juce::Colour color)
        : panelName(name), themeColor(color) {
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        float corner = 5.0f;
        float tabHeight = 25.0f;
        float tabWidth = 120.0f;

        g.setColour(themeColor);
        g.drawRoundedRectangle(area.getX(), area.getY() + tabHeight,
            area.getWidth(), area.getHeight() - tabHeight,
            corner, 2.0f);

        juce::Rectangle<float> tabArea(area.getX(), area.getY(), tabWidth, tabHeight);

        juce::Path tabPath;
        tabPath.addRoundedRectangle(tabArea.getX(), tabArea.getY(), tabArea.getWidth(), tabArea.getHeight() + corner, corner, corner, true, true, false, false);
        g.fillPath(tabPath);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText(panelName, tabArea, juce::Justification::centred, false);
    }

private:
    juce::String panelName;
    juce::Colour themeColor;
};

class ModernLookAndFeel : public juce::LookAndFeel_V4 {
public:

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override {
        return juce::Typeface::createSystemTypefaceFor(juce::Font("Inter", f.getHeight(), f.getStyleFlags()));
    }

    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
        g.setColour(textEditor.findColour(juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 5.0f);
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
        g.setColour(textEditor.findColour(juce::TextEditor::outlineColourId));
        g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f, 5.0f, 1.5f);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider) override {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colour::fromString("#FF1E1E24"));
        g.fillEllipse(rx, ry, rw, rw);

        g.setColour(juce::Colour::fromString("#FF404048"));
        g.drawEllipse(rx, ry, rw, rw, 2.0f);

        g.setColour(juce::Colour::fromString("#FF979B8D").withAlpha(0.5f));
        for (int i = 0; i < 21; ++i) {
            auto tickAngle = rotaryStartAngle + i * (rotaryEndAngle - rotaryStartAngle) / 20.0f;
            juce::Path tick;
            tick.addRectangle(-1.0f, -radius - 4.0f, 2.0f, 4.0f);
            tick.applyTransform(juce::AffineTransform::rotation(tickAngle).translated(centreX, centreY));
            g.fillPath(tick);
        }

        juce::Path p;
        auto pointerLength = radius * 0.6f;
        auto pointerThickness = 3.5f;
        p.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 2.0f, pointerThickness, pointerLength, 1.5f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);
    }
};

class SvgButton : public juce::Button {
public:
    SvgButton(const juce::String& name = "") : juce::Button(name) {}

    void setSvg(const char* svgData, int svgSize) {
        if (svgData != nullptr) {
            icon = juce::Drawable::createFromImageData(svgData, svgSize);
            if (icon) icon->replaceColour(juce::Colours::black, juce::Colours::white);
        }
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = getLocalBounds().toFloat();

        juce::Colour bg = findColour(juce::TextButton::buttonColourId);
        if (getToggleState() || isEnabled() == false) bg = findColour(juce::TextButton::buttonOnColourId);

        if (isEnabled() == false) bg = bg.withAlpha(0.3f);
        else if (shouldDrawButtonAsDown) bg = bg.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted) bg = bg.brighter(0.1f);

        int edges = getConnectedEdgeFlags();
        bool left = (edges & ConnectedOnLeft) == 0;
        bool right = (edges & ConnectedOnRight) == 0;

        juce::Path p;
        p.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 5.0f, 5.0f, left, right, left, right);
        g.setColour(bg);
        g.fillPath(p);

        if (icon) {
            auto iconBounds = bounds.reduced(bounds.getHeight() * 0.25f);
            icon->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
        }
    }
private:
    std::unique_ptr<juce::Drawable> icon;
};