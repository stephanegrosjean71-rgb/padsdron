#include "PluginEditor.h"

namespace
{
const juce::Colour kAccent (0xff7fb7e6);
}

AdpEditor::AdpEditor (AdpProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    laf.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2a3350));
    laf.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    laf.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    laf.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    laf.setColour (juce::ToggleButton::tickColourId, kAccent);
    laf.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1c2340));
    laf.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    laf.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a4570));
    laf.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c2340));
    laf.setColour (juce::PopupMenu::textColourId, juce::Colours::white);

    addSection ("OSCILLATORS",
                { { "saw", "Saw" }, { "sine", "Sine" }, { "fifth", "Fifth" }, { "octave", "Octave" },
                  { "sub", "Sub" }, { "noise", "Air" }, { "detune", "Detune" }, { "width", "Width" },
                  { "drift", "Drift" } });

    addSection ("FILTER  /  MOTION  /  ENVELOPE",
                { { "cutoff", "Cutoff" }, { "reso", "Resonance" }, { "lfoRate", "LFO Rate" },
                  { "lfoDepth", "LFO Depth" }, { "attack", "Attack" }, { "release", "Release" },
                  { "master", "Master" } });

    addSection ("EFFECTS",
                { { "chorus", "Chorus" }, { "delayTime", "Delay Time" }, { "delayFb", "Delay Fdbk" },
                  { "delayMix", "Delay Mix" }, { "reverbDecay", "Reverb Decay" },
                  { "reverbDamp", "Reverb Damp" }, { "reverbMix", "Reverb Mix" } });

    addAndMakeVisible (latch);
    latchAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "latch", latch);

    addAndMakeVisible (presetBox);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };

    setSize (960, 600);
}

AdpEditor::~AdpEditor()
{
    setLookAndFeel (nullptr);
}

void AdpEditor::addSection (const juce::String& title, const std::vector<Def>& defs)
{
    Section section;
    section.title = title;

    for (const auto& d : defs)
    {
        auto k = std::make_unique<Knob>();

        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        if (auto* prm = proc.apvts.getParameter (d.id))
            if (prm->getLabel().isNotEmpty())
                k->slider.setTextValueSuffix (" " + prm->getLabel());

        k->label.setText (d.name, juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setFont (13.0f);

        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);

        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, k->slider);

        section.knobs.push_back (k.get());
        knobs.push_back (std::move (k));
    }

    sections.push_back (std::move (section));
}

void AdpEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b1020), 0.f, 0.f,
                                             juce::Colour (0xff1b1440), 0.f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (24.0f);
    g.drawText ("AMBIENT DRONE PADS", 22, 14, 420, 36, juce::Justification::centredLeft);

    for (const auto& s : sections)
    {
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawRoundedRectangle (s.bounds.toFloat().reduced (2.f), 8.f, 1.f);

        g.setColour (kAccent.withAlpha (0.85f));
        g.setFont (12.0f);
        g.drawText (s.title, s.bounds.getX() + 12, s.bounds.getY() + 6, 420, 16,
                    juce::Justification::centredLeft);
    }
}

void AdpEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto header = area.removeFromTop (50);
    presetBox.setBounds (header.removeFromRight (230).reduced (0, 10));
    header.removeFromRight (10);
    latch.setBounds (header.removeFromRight (160).reduced (0, 10));
    area.removeFromTop (6);

    const int rowH = area.getHeight() / (int) sections.size();

    for (auto& s : sections)
    {
        auto row = area.removeFromTop (rowH);
        s.bounds = row;

        auto inner = row.withTrimmedTop (26);
        const int cw = inner.getWidth() / (int) s.knobs.size();

        for (auto* k : s.knobs)
        {
            auto cell = inner.removeFromLeft (cw);
            k->label.setBounds (cell.removeFromTop (18));
            k->slider.setBounds (cell.reduced (4));
        }
    }
}
