#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list = {
        { "Warm Pad", {} },

        { "Cathedral Drone",
          { { "saw", 0.35f }, { "sine", 0.5f }, { "fifth", 0.3f }, { "octave", 0.25f }, { "sub", 0.4f },
            { "detune", 9.f }, { "cutoff", 1400.f }, { "reso", 0.1f }, { "lfoDepth", 0.25f },
            { "attack", 6.f }, { "release", 14.f }, { "chorus", 0.3f }, { "delayMix", 0.12f },
            { "reverbDecay", 16.f }, { "reverbMix", 0.55f } } },

        { "Glass Shimmer",
          { { "saw", 0.25f }, { "sine", 0.6f }, { "fifth", 0.35f }, { "octave", 0.45f }, { "sub", 0.1f },
            { "detune", 16.f }, { "cutoff", 5200.f }, { "lfoRate", 0.2f }, { "lfoDepth", 0.4f },
            { "attack", 4.f }, { "release", 10.f }, { "chorus", 0.5f }, { "delayMix", 0.3f },
            { "delayFb", 0.55f }, { "reverbDecay", 12.f }, { "reverbMix", 0.5f } } },

        { "Deep Space",
          { { "saw", 0.6f }, { "sine", 0.2f }, { "fifth", 0.2f }, { "octave", 0.f }, { "sub", 0.5f },
            { "detune", 20.f }, { "drift", 0.6f }, { "cutoff", 700.f }, { "reso", 0.25f },
            { "lfoRate", 0.05f }, { "lfoDepth", 0.6f }, { "attack", 9.f }, { "release", 20.f },
            { "delayTime", 780.f }, { "delayMix", 0.3f }, { "reverbDecay", 22.f }, { "reverbMix", 0.6f } } },

        { "Soft Dawn",
          { { "saw", 0.45f }, { "sine", 0.4f }, { "fifth", 0.1f }, { "octave", 0.35f }, { "sub", 0.15f },
            { "cutoff", 4200.f }, { "attack", 2.5f }, { "release", 6.f }, { "chorus", 0.55f },
            { "reverbDecay", 8.f }, { "reverbMix", 0.4f } } },

        { "Endless Drone (Latch)",
          { { "saw", 0.4f }, { "sine", 0.45f }, { "fifth", 0.25f }, { "octave", 0.2f }, { "sub", 0.5f },
            { "drift", 0.5f }, { "cutoff", 1800.f }, { "lfoDepth", 0.35f }, { "attack", 8.f },
            { "release", 18.f }, { "reverbDecay", 20.f }, { "reverbMix", 0.55f }, { "latch", 1.f } } },

        // ---------------- Doux / célestes ----------------
        { "Pure Sine Halo",
          { { "saw", 0.f }, { "sine", 0.9f }, { "fifth", 0.f }, { "octave", 0.15f }, { "sub", 0.f },
            { "noise", 0.f }, { "detune", 4.f }, { "width", 0.6f }, { "drift", 0.15f },
            { "cutoff", 9000.f }, { "reso", 0.f }, { "lfoDepth", 0.05f },
            { "attack", 5.f }, { "release", 9.f }, { "chorus", 0.2f }, { "delayMix", 0.1f },
            { "reverbDecay", 14.f }, { "reverbMix", 0.5f } } },

        { "Angelic Choir",
          { { "saw", 0.3f }, { "sine", 0.5f }, { "fifth", 0.2f }, { "octave", 0.55f }, { "sub", 0.f },
            { "noise", 0.1f }, { "detune", 14.f }, { "width", 1.f }, { "drift", 0.35f },
            { "cutoff", 7500.f }, { "reso", 0.2f }, { "lfoRate", 0.35f }, { "lfoDepth", 0.2f },
            { "attack", 3.f }, { "release", 8.f }, { "chorus", 0.6f }, { "delayMix", 0.2f },
            { "reverbDecay", 13.f }, { "reverbDamp", 9000.f }, { "reverbMix", 0.55f } } },

        { "Sunrise Prayer",
          { { "saw", 0.35f }, { "sine", 0.5f }, { "fifth", 0.15f }, { "octave", 0.45f }, { "sub", 0.1f },
            { "noise", 0.04f }, { "detune", 11.f }, { "width", 0.9f }, { "drift", 0.25f },
            { "cutoff", 6500.f }, { "reso", 0.08f }, { "lfoRate", 0.1f }, { "lfoDepth", 0.15f },
            { "attack", 3.f }, { "release", 7.f }, { "chorus", 0.5f },
            { "delayTime", 500.f }, { "delayFb", 0.4f }, { "delayMix", 0.18f },
            { "reverbDecay", 10.f }, { "reverbDamp", 9000.f }, { "reverbMix", 0.5f } } },

        { "Peaceful Waters",
          { { "saw", 0.2f }, { "sine", 0.6f }, { "fifth", 0.25f }, { "octave", 0.2f }, { "sub", 0.2f },
            { "noise", 0.2f }, { "detune", 8.f }, { "width", 0.9f }, { "drift", 0.4f },
            { "cutoff", 3200.f }, { "lfoRate", 0.2f }, { "lfoDepth", 0.3f },
            { "attack", 4.f }, { "release", 9.f }, { "chorus", 0.65f },
            { "delayTime", 450.f }, { "delayFb", 0.5f }, { "delayMix", 0.22f },
            { "reverbDecay", 11.f }, { "reverbMix", 0.5f } } },

        { "Floating Clouds",
          { { "saw", 0.4f }, { "sine", 0.45f }, { "fifth", 0.15f }, { "octave", 0.3f }, { "sub", 0.15f },
            { "detune", 18.f }, { "width", 1.f }, { "drift", 0.6f }, { "cutoff", 3800.f },
            { "lfoRate", 0.07f }, { "lfoDepth", 0.5f }, { "attack", 5.f }, { "release", 11.f },
            { "chorus", 0.5f }, { "delayTime", 680.f }, { "delayFb", 0.5f }, { "delayMix", 0.28f },
            { "reverbDecay", 12.f }, { "reverbMix", 0.5f } } },

        { "Music Box Mist",
          { { "saw", 0.f }, { "sine", 0.6f }, { "fifth", 0.5f }, { "octave", 0.8f }, { "sub", 0.f },
            { "noise", 0.f }, { "detune", 5.f }, { "width", 1.f }, { "drift", 0.1f },
            { "cutoff", 14000.f }, { "reso", 0.f }, { "lfoDepth", 0.f },
            { "attack", 0.3f }, { "release", 4.f }, { "chorus", 0.25f },
            { "delayTime", 330.f }, { "delayFb", 0.6f }, { "delayMix", 0.4f },
            { "reverbDecay", 9.f }, { "reverbMix", 0.45f } } },

        // ---------------- Sacrés / cosmiques ----------------
        { "Tibetan Bowl Drone",
          { { "saw", 0.1f }, { "sine", 0.6f }, { "fifth", 0.45f }, { "octave", 0.3f }, { "sub", 0.35f },
            { "noise", 0.02f }, { "detune", 3.f }, { "width", 0.7f }, { "drift", 0.2f },
            { "cutoff", 3500.f }, { "reso", 0.35f }, { "lfoRate", 0.08f }, { "lfoDepth", 0.15f },
            { "attack", 3.f }, { "release", 16.f }, { "chorus", 0.1f },
            { "delayTime", 900.f }, { "delayFb", 0.5f }, { "delayMix", 0.25f },
            { "reverbDecay", 18.f }, { "reverbMix", 0.5f }, { "latch", 1.f } } },

        { "Sacred Organ Pad",
          { { "saw", 0.3f }, { "sine", 0.7f }, { "fifth", 0.5f }, { "octave", 0.6f }, { "sub", 0.45f },
            { "noise", 0.f }, { "detune", 2.f }, { "width", 0.5f }, { "drift", 0.05f },
            { "cutoff", 5500.f }, { "reso", 0.05f }, { "lfoDepth", 0.02f },
            { "attack", 0.6f }, { "release", 1.5f }, { "chorus", 0.2f }, { "delayMix", 0.f },
            { "reverbDecay", 11.f }, { "reverbMix", 0.4f } } },

        { "Cosmic Fifths",
          { { "saw", 0.3f }, { "sine", 0.4f }, { "fifth", 0.8f }, { "octave", 0.5f }, { "sub", 0.2f },
            { "detune", 8.f }, { "width", 0.9f }, { "cutoff", 4800.f }, { "lfoRate", 0.1f },
            { "lfoDepth", 0.3f }, { "attack", 4.f }, { "release", 12.f }, { "chorus", 0.4f },
            { "delayTime", 600.f }, { "delayFb", 0.6f }, { "delayMix", 0.35f },
            { "reverbDecay", 15.f }, { "reverbMix", 0.5f } } },

        { "Nordic Fjord",
          { { "saw", 0.5f }, { "sine", 0.2f }, { "fifth", 0.6f }, { "octave", 0.1f }, { "sub", 0.25f },
            { "detune", 6.f }, { "width", 0.8f }, { "drift", 0.45f }, { "cutoff", 2000.f },
            { "reso", 0.45f }, { "lfoRate", 0.11f }, { "lfoDepth", 0.4f },
            { "attack", 4.5f }, { "release", 9.f }, { "chorus", 0.15f },
            { "delayTime", 840.f }, { "delayFb", 0.4f }, { "delayMix", 0.2f },
            { "reverbDecay", 17.f }, { "reverbDamp", 4500.f }, { "reverbMix", 0.5f } } },

        { "Aurora Borealis",
          { { "saw", 0.45f }, { "sine", 0.3f }, { "fifth", 0.45f }, { "octave", 0.5f }, { "sub", 0.1f },
            { "noise", 0.08f }, { "detune", 28.f }, { "width", 1.f }, { "drift", 0.85f },
            { "cutoff", 5000.f }, { "reso", 0.3f }, { "lfoRate", 0.09f }, { "lfoDepth", 0.8f },
            { "attack", 8.f }, { "release", 18.f }, { "chorus", 0.6f },
            { "delayTime", 900.f }, { "delayFb", 0.65f }, { "delayMix", 0.35f },
            { "reverbDecay", 24.f }, { "reverbDamp", 11000.f }, { "reverbMix", 0.6f } } },

        { "Frozen Lake",
          { { "saw", 0.15f }, { "sine", 0.55f }, { "fifth", 0.3f }, { "octave", 0.7f }, { "sub", 0.f },
            { "noise", 0.05f }, { "detune", 6.f }, { "width", 1.f }, { "cutoff", 12000.f },
            { "reso", 0.f }, { "lfoDepth", 0.1f }, { "attack", 7.f }, { "release", 16.f },
            { "chorus", 0.3f }, { "delayTime", 1200.f }, { "delayFb", 0.7f }, { "delayMix", 0.4f },
            { "reverbDecay", 25.f }, { "reverbDamp", 14000.f }, { "reverbMix", 0.6f } } },

        // ---------------- Chauds / analogiques ----------------
        { "Analog String Ensemble",
          { { "saw", 0.85f }, { "sine", 0.1f }, { "fifth", 0.f }, { "octave", 0.3f }, { "sub", 0.1f },
            { "noise", 0.f }, { "detune", 22.f }, { "width", 0.9f }, { "drift", 0.2f },
            { "cutoff", 3000.f }, { "reso", 0.1f }, { "lfoRate", 0.6f }, { "lfoDepth", 0.08f },
            { "attack", 0.9f }, { "release", 2.2f }, { "chorus", 0.8f }, { "delayMix", 0.05f },
            { "reverbDecay", 4.f }, { "reverbMix", 0.25f } } },

        { "Warm Tape Memory",
          { { "saw", 0.55f }, { "sine", 0.3f }, { "fifth", 0.f }, { "octave", 0.1f }, { "sub", 0.3f },
            { "noise", 0.15f }, { "detune", 15.f }, { "width", 0.6f }, { "drift", 0.5f },
            { "cutoff", 1800.f }, { "reso", 0.05f }, { "lfoRate", 0.25f }, { "lfoDepth", 0.12f },
            { "attack", 1.5f }, { "release", 5.f }, { "chorus", 0.7f },
            { "delayTime", 380.f }, { "delayFb", 0.6f }, { "delayMix", 0.4f },
            { "reverbDecay", 5.f }, { "reverbDamp", 3500.f }, { "reverbMix", 0.3f } } },

        // ---------------- Mouvement / texture ----------------
        { "Breathing Wind",
          { { "saw", 0.f }, { "sine", 0.25f }, { "fifth", 0.f }, { "octave", 0.f }, { "sub", 0.f },
            { "noise", 1.f }, { "detune", 10.f }, { "width", 1.f }, { "drift", 0.8f },
            { "cutoff", 1500.f }, { "reso", 0.55f }, { "lfoRate", 0.15f }, { "lfoDepth", 0.9f },
            { "attack", 6.f }, { "release", 10.f }, { "chorus", 0.4f },
            { "reverbDecay", 10.f }, { "reverbMix", 0.5f } } },

        { "Pulse of Light",
          { { "saw", 0.5f }, { "sine", 0.3f }, { "fifth", 0.2f }, { "octave", 0.3f }, { "sub", 0.15f },
            { "cutoff", 2600.f }, { "reso", 0.4f }, { "lfoRate", 1.4f }, { "lfoDepth", 0.7f },
            { "attack", 2.f }, { "release", 5.f }, { "chorus", 0.3f },
            { "delayTime", 375.f }, { "delayFb", 0.55f }, { "delayMix", 0.3f },
            { "reverbDecay", 7.f }, { "reverbMix", 0.4f } } },

        // ---------------- Sombres / puissants ----------------
        { "Cinematic Swell",
          { { "saw", 0.8f }, { "sine", 0.2f }, { "fifth", 0.3f }, { "octave", 0.2f }, { "sub", 0.4f },
            { "detune", 30.f }, { "width", 1.f }, { "drift", 0.4f }, { "cutoff", 1000.f },
            { "reso", 0.2f }, { "lfoRate", 0.04f }, { "lfoDepth", 0.9f },
            { "attack", 15.f }, { "release", 12.f }, { "chorus", 0.35f }, { "delayMix", 0.15f },
            { "reverbDecay", 14.f }, { "reverbMix", 0.45f } } },

        { "Deep Space Tide",
          { { "saw", 0.7f }, { "sine", 0.f }, { "fifth", 0.1f }, { "octave", 0.f }, { "sub", 0.7f },
            { "noise", 0.1f }, { "detune", 25.f }, { "width", 0.9f }, { "drift", 0.7f },
            { "cutoff", 350.f }, { "reso", 0.3f }, { "lfoRate", 0.03f }, { "lfoDepth", 0.7f },
            { "attack", 12.f }, { "release", 25.f }, { "chorus", 0.2f },
            { "delayTime", 1100.f }, { "delayMix", 0.15f },
            { "reverbDecay", 28.f }, { "reverbDamp", 2500.f }, { "reverbMix", 0.65f },
            { "master", -8.f }, { "latch", 1.f } } },

        { "Subterranean Rumble",
          { { "saw", 0.3f }, { "sine", 0.3f }, { "fifth", 0.f }, { "octave", 0.f }, { "sub", 1.f },
            { "noise", 0.3f }, { "detune", 35.f }, { "width", 0.8f }, { "drift", 0.8f },
            { "cutoff", 250.f }, { "reso", 0.5f }, { "lfoRate", 0.05f }, { "lfoDepth", 0.5f },
            { "attack", 10.f }, { "release", 20.f }, { "chorus", 0.1f }, { "delayMix", 0.f },
            { "reverbDecay", 25.f }, { "reverbDamp", 1800.f }, { "reverbMix", 0.5f },
            { "master", -9.f }, { "latch", 1.f } } },

        { "Heavy Industrial Drone",
          { { "saw", 0.95f }, { "sine", 0.f }, { "fifth", 0.5f }, { "octave", 0.f }, { "sub", 0.8f },
            { "noise", 0.4f }, { "detune", 45.f }, { "width", 0.7f }, { "drift", 0.9f },
            { "cutoff", 600.f }, { "reso", 0.6f }, { "lfoRate", 0.06f }, { "lfoDepth", 0.8f },
            { "attack", 7.f }, { "release", 15.f }, { "chorus", 0.1f },
            { "delayTime", 1300.f }, { "delayFb", 0.75f }, { "delayMix", 0.3f },
            { "reverbDecay", 20.f }, { "reverbDamp", 2000.f }, { "reverbMix", 0.5f },
            { "master", -9.f }, { "latch", 1.f } } },
    };
    return list;
}
} // namespace

//==============================================================================
AdpProcessor::AdpProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AdpProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout] (const char* id, const char* name, float lo, float hi, float def,
                          float centre, const char* unit)
    {
        juce::NormalisableRange<float> range (lo, hi);
        if (centre > 0.f)
            range.setSkewForCentre (centre);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def,
            juce::AudioParameterFloatAttributes().withLabel (unit)));
    };

    // Oscillateurs
    add ("saw",    "Saw",    0.f, 1.f, 0.5f,  0.f, "");
    add ("sine",   "Sine",   0.f, 1.f, 0.4f,  0.f, "");
    add ("fifth",  "Fifth",  0.f, 1.f, 0.15f, 0.f, "");
    add ("octave", "Octave", 0.f, 1.f, 0.2f,  0.f, "");
    add ("sub",    "Sub",    0.f, 1.f, 0.25f, 0.f, "");
    add ("noise",  "Air",    0.f, 1.f, 0.05f, 0.f, "");
    add ("detune", "Detune", 0.f, 60.f, 12.f, 0.f, "ct");
    add ("width",  "Width",  0.f, 1.f, 0.8f,  0.f, "");
    add ("drift",  "Drift",  0.f, 1.f, 0.3f,  0.f, "");

    // Filtre / modulation / enveloppe
    add ("cutoff",   "Cutoff",    60.f, 18000.f, 2200.f, 1500.f, "Hz");
    add ("reso",     "Resonance", 0.f, 0.9f, 0.15f, 0.f, "");
    add ("lfoRate",  "LFO Rate",  0.01f, 2.f, 0.12f, 0.2f, "Hz");
    add ("lfoDepth", "LFO Depth", 0.f, 1.f, 0.3f, 0.f, "");
    add ("attack",   "Attack",    0.05f, 20.f, 3.5f, 2.5f, "s");
    add ("release",  "Release",   0.2f, 30.f, 8.f, 6.f, "s");
    add ("master",   "Master",    -24.f, 6.f, -6.f, 0.f, "dB");

    // Effets
    add ("chorus",      "Chorus",         0.f, 1.f, 0.4f, 0.f, "");
    add ("delayTime",   "Delay Time",     120.f, 1500.f, 520.f, 500.f, "ms");
    add ("delayFb",     "Delay Feedback", 0.f, 0.85f, 0.45f, 0.f, "");
    add ("delayMix",    "Delay Mix",      0.f, 1.f, 0.2f, 0.f, "");
    add ("reverbDecay", "Reverb Decay",   1.f, 30.f, 9.f, 7.f, "s");
    add ("reverbDamp",  "Reverb Damping", 1500.f, 16000.f, 7000.f, 5500.f, "Hz");
    add ("reverbMix",   "Reverb Mix",     0.f, 1.f, 0.45f, 0.f, "");

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "latch", 1 }, "Latch (Drone)", false));

    return layout;
}

adp::Params AdpProcessor::readParams() const
{
    adp::Params p;
    p.saw = get ("saw");         p.sine = get ("sine");       p.fifth = get ("fifth");
    p.octave = get ("octave");   p.sub = get ("sub");         p.noise = get ("noise");
    p.detune = get ("detune");   p.width = get ("width");     p.drift = get ("drift");
    p.cutoff = get ("cutoff");   p.reso = get ("reso");       p.lfoRate = get ("lfoRate");
    p.lfoDepth = get ("lfoDepth");
    p.attack = get ("attack");   p.release = get ("release");
    return p;
}

//==============================================================================
void AdpProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, readParams());
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    chorus.prepare (spec);
    chorus.setRate (0.3f);
    chorus.setDepth (0.35f);
    chorus.setCentreDelay (9.0f);
    chorus.setFeedback (0.0f);
    chorus.reset();

    masterGain.reset (sampleRate, 0.05);
    masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (get ("master")));
}

bool AdpProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AdpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2)
        return;

    const adp::Params p = readParams();
    engine.setLatch (get ("latch") > 0.5f);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    int pos = 0;
    auto renderTo = [&] (int end)
    {
        while (pos < end)
        {
            const int c = std::min (adp::Engine::controlSize, end - pos);
            engine.render (L + pos, R + pos, c, p);
            pos += c;
        }
    };

    for (const auto metadata : midi)
    {
        renderTo (metadata.samplePosition);
        const auto m = metadata.getMessage();

        if (m.isNoteOn())                              engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                        engine.noteOff (m.getNoteNumber());
        else if (m.isSustainPedalOn())                 engine.sustain (true);
        else if (m.isSustainPedalOff())                engine.sustain (false);
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }
    renderTo (numSamples);

    // --- effets : chorus -> delay -> réverb -> master ---
    chorus.setMix (get ("chorus"));
    juce::dsp::AudioBlock<float> block (buffer);
    chorus.process (juce::dsp::ProcessContextReplacing<float> (block));

    delay.set (get ("delayTime"), get ("delayFb"), get ("delayMix"));
    reverb.setParams (get ("reverbDecay"), get ("reverbDamp"), get ("reverbMix"));
    masterGain.setTargetValue (juce::Decibels::decibelsToGain (get ("master")));

    for (int i = 0; i < numSamples; ++i)
    {
        float l = L[i], r = R[i];
        delay.process (l, r);
        reverb.process (l, r);
        const float g = masterGain.getNextValue();
        L[i] = std::tanh (l * g); // limiteur doux de sécurité
        R[i] = std::tanh (r * g);
    }
}

//==============================================================================
juce::AudioProcessorEditor* AdpProcessor::createEditor() { return new AdpEditor (*this); }

int AdpProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String AdpProcessor::getProgramName (int index)
{
    const auto& list = presets();
    return (index >= 0 && index < (int) list.size()) ? juce::String (list[(size_t) index].name) : juce::String();
}

void AdpProcessor::setCurrentProgram (int index)
{
    const auto& list = presets();
    if (index < 0 || index >= (int) list.size())
        return;

    currentProgram = index;

    for (auto* prm : getParameters())
        prm->setValueNotifyingHost (prm->getDefaultValue());

    for (const auto& kv : list[(size_t) index].values)
        if (auto* rp = apvts.getParameter (kv.first))
            rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second));
}

void AdpProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AdpProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AdpProcessor(); }
