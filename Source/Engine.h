#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace adp
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

//==============================================================================
struct Params
{
    float saw = 0.5f, sine = 0.4f, fifth = 0.15f, octave = 0.2f, sub = 0.25f, noise = 0.05f;
    float detune = 12.f, width = 0.8f, drift = 0.3f;
    float cutoff = 2200.f, reso = 0.15f, lfoRate = 0.12f, lfoDepth = 0.3f;
    float attack = 3.5f, release = 8.f;
};

//==============================================================================
// Filtre passe-bas SVF (topologie TPT, très stable en modulation)
struct Svf2
{
    float ic1[2] {}, ic2[2] {};
    float a1 = 0.f, a2 = 0.f, a3 = 0.f;

    void setCutoff (float cutoff, float fs, float res)
    {
        const float g = std::tan (kPi * cutoff / fs);
        const float k = 2.0f - 1.9f * res;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float lowpass (int ch, float v0)
    {
        const float v3 = v0 - ic2[ch];
        const float v1 = a1 * ic1[ch] + a2 * v3;
        const float v2 = ic2[ch] + a2 * ic1[ch] + a3 * v3;
        ic1[ch] = 2.f * v1 - ic1[ch];
        ic2[ch] = 2.f * v2 - ic2[ch];
        return v2;
    }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)         { t /= dt;               return t + t - t * t - 1.f; }
    if (t > 1.f - dt)   { t = (t - 1.f) / dt;    return t * t + t + t + 1.f; }
    return 0.f;
}

// Dérive lente aléatoire (donne de la vie aux oscillateurs et au filtre)
struct Wander
{
    float value = 0.f, target = 0.f;
    int counter = 0;

    float step (juce::Random& r, int n, float fs, float seconds)
    {
        counter -= n;
        if (counter <= 0)
        {
            target = r.nextFloat() * 2.f - 1.f;
            counter = (int) (seconds * fs * (0.5f + r.nextFloat()));
        }
        const float c = std::min (1.f, (float) n / (0.7f * seconds * fs));
        value += (target - value) * c;
        return value;
    }
};

//==============================================================================
struct Voice
{
    bool active = false, keyDown = false, latched = false, pedalHeld = false;
    int note = -1;
    uint64_t age = 0;
    float env = 0.f, vel = 1.f;
    float phase[3] {}, phaseFifth[3] {}, phaseSub = 0.f, lfoPhase = 0.f;
    float noiseLpL = 0.f, noiseLpR = 0.f;
    Svf2 filter;
    Wander wander[3], wanderFilter;

    bool gate() const { return keyDown || latched || pedalHeld; }
};

//==============================================================================
class Engine
{
public:
    static constexpr int numVoices = 8;
    static constexpr int controlSize = 32;

    void prepare (double sampleRate, const Params& initial)
    {
        fs = (float) sampleRate;
        cur = initial;
        for (auto& v : voices) v = Voice {};
        sustainDown = false;
    }

    void setLatch (bool on)
    {
        if (latchOn && ! on)
            for (auto& v : voices) v.latched = false;
        latchOn = on;
    }

    void sustain (bool down)
    {
        sustainDown = down;
        if (! down)
            for (auto& v : voices) v.pedalHeld = false;
    }

    void allNotesOff()
    {
        for (auto& v : voices) { v.keyDown = false; v.latched = false; v.pedalHeld = false; }
    }

    void noteOn (int note, float velocity)
    {
        // Mode drone : rejouer une note verrouillée la relâche
        if (latchOn)
            for (auto& v : voices)
                if (v.active && v.note == note && v.latched)
                {
                    v.latched = false; v.keyDown = false; v.pedalHeld = false;
                    return;
                }

        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) { target = &v; break; }
        if (target == nullptr)
            for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            for (auto& v : voices)
            {
                if (target == nullptr) { target = &v; continue; }
                const bool vg = v.gate(), cg = target->gate();
                if (cg && ! vg) target = &v;
                else if (cg == vg && v.age < target->age) target = &v;
            }
        }

        if (! target->active)
        {
            *target = Voice {};
            for (int u = 0; u < 3; ++u) { target->phase[u] = rng.nextFloat(); target->phaseFifth[u] = rng.nextFloat(); }
            target->phaseSub = rng.nextFloat();
            target->lfoPhase = rng.nextFloat();
        }

        target->active = true;
        target->note = note;
        target->keyDown = true;
        target->latched = latchOn;
        target->pedalHeld = false;
        target->vel = 0.6f + 0.4f * velocity;
        target->age = ++ageCounter;
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note && v.keyDown)
            {
                v.keyDown = false;
                if (sustainDown) v.pedalHeld = true;
            }
    }

    // Ajoute n échantillons (n <= controlSize) dans L et R
    void render (float* L, float* R, int n, const Params& t)
    {
        smooth (t);
        const float attCoef = std::exp (-1.f / (std::max (0.01f, t.attack) / 6.f * fs));
        const float relCoef = std::exp (-1.f / (std::max (0.05f, t.release) / 7.f * fs));

        for (auto& v : voices)
            if (v.active)
                renderVoice (v, L, R, n, attCoef, relCoef);
    }

private:
    void smooth (const Params& t)
    {
        auto s = [] (float& c, float tg) { c += (tg - c) * 0.25f; };
        s (cur.saw, t.saw);       s (cur.sine, t.sine);     s (cur.fifth, t.fifth);
        s (cur.octave, t.octave); s (cur.sub, t.sub);       s (cur.noise, t.noise);
        s (cur.detune, t.detune); s (cur.width, t.width);   s (cur.drift, t.drift);
        s (cur.cutoff, t.cutoff); s (cur.reso, t.reso);     s (cur.lfoRate, t.lfoRate);
        s (cur.lfoDepth, t.lfoDepth);
    }

    void renderVoice (Voice& v, float* L, float* R, int n, float attCoef, float relCoef)
    {
        const float baseFreq = 440.f * std::pow (2.f, (float) (v.note - 69) / 12.f);

        // --- modulations à la cadence de contrôle ---
        v.lfoPhase += cur.lfoRate * (float) n / fs;
        if (v.lfoPhase >= 1.f) v.lfoPhase -= 1.f;
        const float lfo = std::sin (kTwoPi * v.lfoPhase);
        const float fDrift = v.wanderFilter.step (rng, n, fs, 5.f);

        static const float pos[3] = { -1.f, 0.f, 1.f };
        float inc[3], gl[3], gr[3];
        for (int u = 0; u < 3; ++u)
        {
            const float w = v.wander[u].step (rng, n, fs, 4.f);
            const float cents = pos[u] * cur.detune + w * cur.drift * 9.f;
            inc[u] = baseFreq * std::pow (2.f, cents / 1200.f) / fs;
            const float a = (pos[u] * cur.width + 1.f) * kPi * 0.25f;
            gl[u] = std::cos (a);
            gr[u] = std::sin (a);
        }
        const float subInc = baseFreq * 0.5f / fs;

        float cut = cur.cutoff * std::pow (baseFreq / 261.63f, 0.4f)
                  * std::pow (2.f, 2.f * cur.lfoDepth * lfo + 1.2f * cur.drift * fDrift);
        cut = juce::jlimit (30.f, std::min (18000.f, 0.45f * fs), cut);
        v.filter.setCutoff (cut, fs, cur.reso);

        const bool gate = v.gate();

        for (int i = 0; i < n; ++i)
        {
            // --- enveloppe ---
            if (gate)
                v.env = (v.env < 0.9995f) ? 1.f - (1.f - v.env) * attCoef : 1.f;
            else
            {
                v.env *= relCoef;
                if (v.env < 1e-4f) { v.active = false; v.env = 0.f; break; }
            }

            // --- oscillateurs (3 unissons x [saw, sinus, quinte, octave]) ---
            float oL = 0.f, oR = 0.f;
            for (int u = 0; u < 3; ++u)
            {
                float& ph = v.phase[u];
                float& pf = v.phaseFifth[u];
                const float sSaw = 2.f * ph - 1.f - polyBlep (ph, inc[u]);
                const float sSin = std::sin (kTwoPi * ph);
                const float sOct = std::sin (2.f * kTwoPi * ph);
                const float sFif = std::sin (kTwoPi * pf);
                const float s = cur.saw * sSaw + cur.sine * sSin + cur.octave * sOct + cur.fifth * sFif;
                oL += s * gl[u];
                oR += s * gr[u];

                ph += inc[u];            if (ph >= 1.f) ph -= 1.f;
                pf += inc[u] * 1.4983071f; if (pf >= 1.f) pf -= 1.f;
            }
            oL *= 0.16f;
            oR *= 0.16f;

            // --- sub + souffle (bruit filtré) ---
            const float sub = std::sin (kTwoPi * v.phaseSub) * cur.sub * 0.3f;
            v.phaseSub += subInc; if (v.phaseSub >= 1.f) v.phaseSub -= 1.f;

            v.noiseLpL += 0.15f * ((rng.nextFloat() * 2.f - 1.f) - v.noiseLpL);
            v.noiseLpR += 0.15f * ((rng.nextFloat() * 2.f - 1.f) - v.noiseLpR);
            oL += sub + v.noiseLpL * cur.noise * 0.35f;
            oR += sub + v.noiseLpR * cur.noise * 0.35f;

            // --- filtre + amplitude ---
            oL = v.filter.lowpass (0, oL);
            oR = v.filter.lowpass (1, oR);
            const float amp = v.env * v.env * v.vel;
            L[i] += oL * amp;
            R[i] += oR * amp;
        }
    }

    float fs = 44100.f;
    Params cur;
    juce::Random rng { 0x5eed1234 };
    uint64_t ageCounter = 0;
    bool latchOn = false, sustainDown = false;
    std::array<Voice, numVoices> voices;
};

//==============================================================================
// Délai stéréo croisé (ping-pong doux), feedback filtré
class StereoDelay
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        size = (int) (fs * 2.2f) + 8;
        bufL.assign ((size_t) size, 0.f);
        bufR.assign ((size_t) size, 0.f);
        wr = 0; lpL = lpR = 0.f;
        lpCoef = 1.f - std::exp (-kTwoPi * 3500.f / fs);
        timeS = timeT = 0.5f * fs;
        mixS = mixT = 0.f;
    }

    void set (float timeMs, float feedback, float mix)
    {
        timeT = timeMs * 0.001f * fs;
        fb = feedback;
        mixT = mix;
    }

    void process (float& l, float& r)
    {
        timeS += (timeT - timeS) * 0.0005f;
        mixS += (mixT - mixS) * 0.001f;

        const float dL = readLerp (bufL, (float) wr - timeS);
        const float dR = readLerp (bufR, (float) wr - timeS);
        lpL += lpCoef * (dL - lpL);
        lpR += lpCoef * (dR - lpR);

        bufL[(size_t) wr] = l + lpR * fb;
        bufR[(size_t) wr] = r + lpL * fb;
        wr = (wr + 1) % size;

        l += dL * mixS;
        r += dR * mixS;
    }

private:
    float readLerp (const std::vector<float>& b, float p) const
    {
        while (p < 0.f) p += (float) size;
        int i0 = (int) p;
        const float f = p - (float) i0;
        i0 %= size;
        const int i1 = (i0 + 1) % size;
        return b[(size_t) i0] * (1.f - f) + b[(size_t) i1] * f;
    }

    float fs = 44100.f, timeS = 0.f, timeT = 0.f, mixS = 0.f, mixT = 0.f, fb = 0.4f;
    float lpL = 0.f, lpR = 0.f, lpCoef = 0.3f;
    int size = 1, wr = 0;
    std::vector<float> bufL, bufR;
};

//==============================================================================
// Réverbération FDN 8 lignes (matrice de Householder), lignes modulées, amortissement
class FdnReverb
{
public:
    static constexpr int N = 8;

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        static const int base[N] = { 1327, 1559, 1823, 2129, 2459, 2801, 3203, 3631 };
        for (int i = 0; i < N; ++i)
        {
            len[i] = (int) ((float) base[i] * fs / 44100.f);
            buf[(size_t) i].assign ((size_t) len[i] + 32, 0.f);
            wr[i] = 0;
            lp[i] = 0.f;
            lfoPh[i] = 0.125f * (float) i;
        }
        mixS = mixT = 0.f;
        setParams (9.f, 7000.f, 0.f);
    }

    void setParams (float decaySec, float dampHz, float mix)
    {
        float gSum = 0.f;
        for (int i = 0; i < N; ++i)
        {
            gain[i] = std::pow (10.f, -3.f * (float) len[i] / (fs * std::max (0.1f, decaySec)));
            gSum += gain[i];
        }
        const float gAvg = gSum / (float) N;
        inGain = 0.5f * std::sqrt (std::max (0.01f, 1.f - gAvg * gAvg));
        damp = 1.f - std::exp (-kTwoPi * dampHz / fs);
        mixT = mix;
    }

    void process (float& l, float& r)
    {
        mixS += (mixT - mixS) * 0.001f;
        const float in = (l + r) * 0.5f * inGain;

        float d[N];
        float sum = 0.f;
        for (int i = 0; i < N; ++i)
        {
            lfoPh[i] += (0.05f + 0.011f * (float) i) / fs;
            if (lfoPh[i] >= 1.f) lfoPh[i] -= 1.f;

            const int sz = (int) buf[(size_t) i].size();
            float rp = (float) wr[i] - ((float) len[i] + 5.f * std::sin (kTwoPi * lfoPh[i]));
            if (rp < 0.f) rp += (float) sz;
            const int i0 = ((int) rp) % sz;
            const float f = rp - std::floor (rp);
            const int i1 = (i0 + 1) % sz;
            const float x = buf[(size_t) i][(size_t) i0] * (1.f - f) + buf[(size_t) i][(size_t) i1] * f;

            lp[i] += damp * (x - lp[i]);
            d[i] = lp[i] * gain[i];
            sum += d[i];
        }

        const float h = sum * (2.f / (float) N);
        for (int i = 0; i < N; ++i)
        {
            buf[(size_t) i][(size_t) wr[i]] = d[i] - h + in;
            wr[i] = (wr[i] + 1) % (int) buf[(size_t) i].size();
        }

        const float wetL = 1.5f * 0.5f * (d[0] - d[2] + d[4] - d[6]);
        const float wetR = 1.5f * 0.5f * (d[1] - d[3] + d[5] - d[7]);

        const float cd = std::cos (mixS * kPi * 0.5f);
        const float cw = std::sin (mixS * kPi * 0.5f);
        l = l * cd + wetL * cw;
        r = r * cd + wetR * cw;
    }

private:
    float fs = 44100.f, damp = 0.5f, inGain = 0.1f, mixS = 0.f, mixT = 0.f;
    int len[N] {}, wr[N] {};
    float gain[N] {}, lp[N] {}, lfoPh[N] {};
    std::array<std::vector<float>, N> buf;
};

} // namespace adp
