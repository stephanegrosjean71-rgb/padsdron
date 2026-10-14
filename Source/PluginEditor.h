#pragma once

#include "PluginProcessor.h"

class AdpEditor : public juce::AudioProcessorEditor
{
public:
    explicit AdpEditor (AdpProcessor&);
    ~AdpEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Def { const char* id; const char* name; };

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct Section
    {
        juce::String title;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds;
    };

    void addSection (const juce::String& title, const std::vector<Def>& defs);

    AdpProcessor& proc;
    juce::LookAndFeel_V4 laf;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Section> sections;

    juce::ToggleButton latch { "Latch / Drone" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAtt;
    juce::ComboBox presetBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdpEditor)
};
