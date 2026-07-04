#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// The Ondaire synthesis engine.
//
// Signal path, per voice (mirroring the Ondioline's chain):
//
//   multivibrator osc ("tops" pulse or square, lever B)
//     -> pentode preamp soft clipping (bypassed by lever A)
//     -> lever F  : series capacitor, one-pole high-pass ("sharpen")
//     -> lever C  : one-pole low-pass ("blunt the impulse")
//     -> caps E/I/J/K without a coil: tone-loading low-pass
//     -> resonators G and H (band-pass "formant" coils), tuned by the
//        engaged capacitor levers E/I/J/K, mixed with a dry bleed
//     -> modern resonant multimode filter (LP/HP/BP state-variable)
//     -> envelope: ADSR, or plucked capacitor discharge when lever P is down
//
// Globally: vibrato LFO (levers V1/V2/W + modern rate/depth + mod wheel),
// chopped tremolo (lever D), knee-lever expression, output gain.
namespace ondaire
{

//==============================================================================
// A snapshot of every parameter the engine needs, taken once per block.
struct Snapshot
{
    // Legacy
    int   octaveShift  = 0;       // derived from the register switch, -2..+1
    float tuneCents    = 0.0f;
    float pulseWidth   = 0.12f;
    bool  levA = false, levB = false, levC = false, levD = false;
    bool  levE = false, levF = false, levG = false, levH = false;
    bool  levI = false, levJ = false, levK = false, levM = false;
    bool  levP = false, levV1 = false, levV2 = false, levW = false;
    float percDecay    = 0.6f;
    float drive        = 0.35f;
    float noise        = 0.0f;

    // Modern
    int   filterType   = 0;       // 0 off, 1 LP, 2 HP, 3 BP
    float cutoff       = 4000.0f;
    float resonance    = 0.15f;
    float attack       = 0.012f, decay = 0.15f, sustain = 0.9f, release = 0.08f;
    bool  mono         = true;
    float glide        = 0.03f;
    float pbRange      = 2.0f;
    float vibRate      = 5.2f;
    float vibDepth     = 0.0f;    // cents
    float tremRate     = 9.0f;
    float tremDepth    = 0.85f;
    float expression   = 1.0f;
    float gainLin      = 0.5f;
};

//==============================================================================
// Zero-delay-feedback state variable filter (TPT topology).
struct Svf
{
    void reset()                       { ic1 = ic2 = 0.0f; }
    void prepare (double sampleRate)   { sr = sampleRate; reset(); }

    void set (float freq, float q)
    {
        freq = juce::jlimit (20.0f, (float) (sr * 0.45), freq);
        g = std::tan (juce::MathConstants<float>::pi * freq / (float) sr);
        k = 1.0f / juce::jmax (0.25f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Processes one sample, returning {lp, bp, hp}.
    void process (float x, float& lp, float& bp, float& hp)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }

    float bandPass (float x)  { float l, b, h; process (x, l, b, h); return b; }

    double sr = 44100.0;
    float g = 0.1f, k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
};

//==============================================================================
struct OnePole
{
    void reset()                     { z = 0.0f; }
    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void setCutoff (float freq)
    {
        freq = juce::jlimit (10.0f, (float) (sr * 0.45), freq);
        a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * freq / (float) sr);
    }
    float lowPass  (float x)  { z += a * (x - z); return z; }
    float highPass (float x)  { return x - lowPass (x); }

    double sr = 44100.0;
    float a = 0.5f, z = 0.0f;
};

//==============================================================================
// One synthesis voice.
class Voice
{
public:
    void prepare (double sampleRate);
    void noteOn  (int midiNote, float velocity, bool legato);
    void noteOff (bool allowTailOff);
    void steal()                       { noteOff (true); }
    void kill();

    bool isActive() const              { return active; }
    bool isHeld()   const              { return held; }
    int  getNote()  const              { return note; }
    juce::int64 getStartTime() const   { return startTime; }
    void setStartTime (juce::int64 t)  { startTime = t; }

    // Renders and adds this voice into mono buffer out[0..n).
    // vibratoCents holds the per-sample global vibrato offset.
    void render (float* out, const float* vibratoCents, int n,
                 const Snapshot& s, float pitchBendSemis, float pressure);

private:
    void updateFilters (const Snapshot& s);

    double sr = 44100.0;
    bool   active = false, held = false;
    int    note = -1;
    float  velocity = 1.0f;
    juce::int64 startTime = 0;

    // Oscillator
    float phase = 0.0f;
    float currentFreq = 440.0f, targetFreq = 440.0f;

    // Envelopes
    juce::ADSR adsr;
    float pluckLevel = 0.0f;      // percussion capacitor discharge
    float burstLevel = 0.0f;      // lever M string-tap noise chiff

    // Filters
    OnePole hpF, lpC, lpCaps;
    Svf resG, resH, modern;

    juce::Random rng;
};

//==============================================================================
class Engine
{
public:
    static constexpr int maxVoices = 8;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Renders into a stereo (or mono) buffer, consuming MIDI.
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                  const Snapshot& snapshot);

private:
    void handleMidiEvent (const juce::MidiMessage& m, const Snapshot& s);
    void renderSegment (int startSample, int numSamples, const Snapshot& s);

    void noteOn  (int note, float velocity, const Snapshot& s);
    void noteOff (int note, const Snapshot& s);
    Voice* findFreeVoice();

    double sr = 44100.0;
    std::array<Voice, maxVoices> voices;
    juce::AudioBuffer<float> monoBus;
    std::vector<float> vibratoCents;

    // Mono mode note stack (most recent last).
    std::vector<int>   heldNotes;
    std::vector<float> heldVels;

    // Controller state
    float pitchBend   = 0.0f;   // semitones
    float modWheel    = 0.0f;   // 0..1, adds vibrato
    float exprCC      = 1.0f;   // CC11
    float pressure    = 0.0f;   // channel aftertouch
    float cc74        = 0.5f;   // brightness, scales cutoff
    float cc71        = 0.0f;   // adds resonance
    bool  sustainDown = false;
    std::array<bool, 128> sustained {};

    // Global LFOs
    float vibPhase = 0.0f, tremPhase = 0.0f;

    // Smoothers
    juce::SmoothedValue<float> exprSmooth { 1.0f }, gainSmooth { 0.5f };

    juce::int64 noteCounter = 0;
    bool wasMono = true;

public:
    // Modulated filter values, computed by process() from snapshot + CCs.
    float effectiveCutoff (const Snapshot& s) const
    {
        // CC74 sweeps the cutoff +/- two octaves around the parameter value.
        return s.cutoff * std::exp2 ((cc74 - 0.5f) * 4.0f);
    }
    float effectiveResonance (const Snapshot& s) const
    {
        return juce::jmin (1.0f, s.resonance + cc71);
    }
};

} // namespace ondaire
