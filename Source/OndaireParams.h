#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs for the Ondaire.
//
// The "legacy" section mirrors the controls of the original Ondioline as
// described in Georges Jenny's construction manual:
//   - the "cle d'octaves" register switch (4 positions, transposing octave
//     by octave),
//   - the general tuning potentiometer ("bouton d'accord general"),
//   - the bank of timbre levers A..W mounted under the keyboard.
//
// The "modern" section adds what the user of a 21st-century plugin expects:
// resonant multimode filtering, a full ADSR, mono/poly voicing, glide and a
// configurable pitch-bend range (the pitch wheel replaces the Ondioline's
// laterally-oscillating keyboard for manual vibrato).
namespace ondaire::param
{
    // Legacy: oscillator & register -------------------------------------
    inline constexpr auto registre   = "register";     // cle d'octaves, 4 positions
    inline constexpr auto tune       = "tune";          // accord general, cents
    inline constexpr auto pulseWidth = "pulseWidth";    // width of the "tops" impulse

    // Legacy: timbre lever bank ------------------------------------------
    // Lever letters follow the manual. L is omitted (reserved, unused in the
    // standard model); M is included (string-tap percussion).
    inline constexpr auto leverA  = "leverA";   // bypass preamp pentode (removes drive stage)
    inline constexpr auto leverB  = "leverB";   // waveform: up = "tops" pulse, down = square (odd harmonics)
    inline constexpr auto leverC  = "leverC";   // low-pass, blunts the exciting impulse
    inline constexpr auto leverD  = "leverD";   // chopped tremolo (mandolin / banjo repetition)
    inline constexpr auto leverE  = "leverE";   // resonator tuning capacitor E
    inline constexpr auto leverF  = "leverF";   // series capacitor, sharpens the impulse (high-pass)
    inline constexpr auto leverG  = "leverG";   // formant resonator coil G
    inline constexpr auto leverH  = "leverH";   // formant resonator coil H
    inline constexpr auto leverI  = "leverI";   // resonator tuning capacitor I
    inline constexpr auto leverJ  = "leverJ";   // resonator tuning capacitor J
    inline constexpr auto leverK  = "leverK";   // resonator tuning capacitor K
    inline constexpr auto leverM  = "leverM";   // string-tap percussion (noise chiff)
    inline constexpr auto leverP  = "leverP";   // percussion: plucked-string envelope
    inline constexpr auto leverV1 = "leverV1";  // automatic vibrato, deep
    inline constexpr auto leverV2 = "leverV2";  // automatic vibrato, shallow
    inline constexpr auto leverW  = "leverW";   // vibrato speed (down = fast)

    inline constexpr auto percDecay = "percDecay";  // discharge time of the percussion capacitor
    inline constexpr auto drive     = "drive";      // preamp pentode drive amount (defeated by lever A)
    inline constexpr auto noise     = "noise";      // "souffle" breath noise level

    // Modern: filter -----------------------------------------------------
    inline constexpr auto filterType = "filterType";  // Off / LP / HP / BP
    inline constexpr auto cutoff     = "cutoff";
    inline constexpr auto resonance  = "resonance";

    // Modern: envelope ----------------------------------------------------
    inline constexpr auto attack  = "attack";
    inline constexpr auto decay   = "decay";
    inline constexpr auto sustain = "sustain";
    inline constexpr auto release = "release";

    // Modern: voicing ------------------------------------------------------
    inline constexpr auto mode    = "mode";      // Mono (authentic) / Poly
    inline constexpr auto glide   = "glide";     // portamento time, mono mode
    inline constexpr auto pbRange = "pbRange";   // pitch wheel range in semitones

    // Modern: modulation ---------------------------------------------------
    inline constexpr auto vibRate   = "vibRate";
    inline constexpr auto vibDepth  = "vibDepth";
    inline constexpr auto tremRate  = "tremRate";
    inline constexpr auto tremDepth = "tremDepth";

    // Output ----------------------------------------------------------------
    inline constexpr auto expression = "expression";  // the "genouillere" knee lever
    inline constexpr auto gain       = "gain";

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        std::vector<std::unique_ptr<RangedAudioParameter>> p;

        auto boolParam = [&] (const char* id, const String& name, bool def)
        {
            p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
        };

        // Legacy ------------------------------------------------------------
        p.push_back (std::make_unique<AudioParameterChoice> (
            ParameterID { registre, 1 }, "Octave Key (Register)",
            StringArray { "1 (Bass)", "2", "3", "4 (High)" }, 2));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { tune, 1 }, "Tune",
            NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 0.0f,
            AudioParameterFloatAttributes().withLabel ("ct")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { pulseWidth, 1 }, "Tops Width",
            NormalisableRange<float> (0.05f, 0.45f, 0.01f), 0.12f));

        boolParam (leverA,  "Lever A (No Drive)",       false);
        boolParam (leverB,  "Lever B (Square Wave)",    false);
        boolParam (leverC,  "Lever C (Low-Pass)",       false);
        boolParam (leverD,  "Lever D (Chop Tremolo)",   false);
        boolParam (leverE,  "Lever E (Tuning Cap E)",   false);
        boolParam (leverF,  "Lever F (Sharpen)",        false);
        boolParam (leverG,  "Lever G (Resonator G)",    false);
        boolParam (leverH,  "Lever H (Resonator H)",    false);
        boolParam (leverI,  "Lever I (Tuning Cap I)",   false);
        boolParam (leverJ,  "Lever J (Tuning Cap J)",   false);
        boolParam (leverK,  "Lever K (Tuning Cap K)",   false);
        boolParam (leverM,  "Lever M (String Tap)",     false);
        boolParam (leverP,  "Lever P (Percussion)",     false);
        boolParam (leverV1, "Lever V1 (Vibrato Deep)",  false);
        boolParam (leverV2, "Lever V2 (Vibrato Soft)",  false);
        boolParam (leverW,  "Lever W (Vibrato Fast)",   false);

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { percDecay, 1 }, "Percussion Decay",
            NormalisableRange<float> (0.05f, 3.0f, 0.05f, 0.4f), 0.6f,
            AudioParameterFloatAttributes().withLabel ("s")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { drive, 1 }, "Pentode Drive",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { noise, 1 }, "Souffle (Breath)",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // Modern filter --------------------------------------------------------
        p.push_back (std::make_unique<AudioParameterChoice> (
            ParameterID { filterType, 1 }, "Filter Type",
            StringArray { "Off", "Low-Pass", "High-Pass", "Band-Pass" }, 0));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { cutoff, 1 }, "Filter Cutoff",
            NormalisableRange<float> (20.0f, 20000.0f, 10.0f, 0.25f), 4000.0f,
            AudioParameterFloatAttributes().withLabel ("Hz")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { resonance, 1 }, "Filter Resonance",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));

        // Envelope ---------------------------------------------------------
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { attack, 1 }, "Attack",
            NormalisableRange<float> (0.001f, 3.0f, 0.01f, 0.35f), 0.012f,
            AudioParameterFloatAttributes().withLabel ("s")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { decay, 1 }, "Decay",
            NormalisableRange<float> (0.001f, 4.0f, 0.01f, 0.35f), 0.15f,
            AudioParameterFloatAttributes().withLabel ("s")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { sustain, 1 }, "Sustain",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.9f));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { release, 1 }, "Release",
            NormalisableRange<float> (0.005f, 6.0f, 0.01f, 0.35f), 0.08f,
            AudioParameterFloatAttributes().withLabel ("s")));

        // Voicing ------------------------------------------------------------
        p.push_back (std::make_unique<AudioParameterChoice> (
            ParameterID { mode, 1 }, "Voice Mode",
            StringArray { "Mono (Legacy)", "Poly" }, 0));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { glide, 1 }, "Glide",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f, 0.4f), 0.03f,
            AudioParameterFloatAttributes().withLabel ("s")));

        p.push_back (std::make_unique<AudioParameterInt> (
            ParameterID { pbRange, 1 }, "Pitch Bend Range", 1, 12, 2,
            AudioParameterIntAttributes().withLabel ("st")));

        // Modulation --------------------------------------------------------
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { vibRate, 1 }, "Vibrato Rate",
            NormalisableRange<float> (3.0f, 10.0f, 0.1f), 5.2f,
            AudioParameterFloatAttributes().withLabel ("Hz")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { vibDepth, 1 }, "Vibrato Depth",
            NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f,
            AudioParameterFloatAttributes().withLabel ("ct")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { tremRate, 1 }, "Tremolo Rate",
            NormalisableRange<float> (3.0f, 16.0f, 0.1f), 9.0f,
            AudioParameterFloatAttributes().withLabel ("Hz")));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { tremDepth, 1 }, "Tremolo Depth",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.85f));

        // Output ----------------------------------------------------------------
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { expression, 1 }, "Expression (Knee Lever)",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { gain, 1 }, "Output Gain",
            NormalisableRange<float> (-36.0f, 6.0f, 0.5f), -6.0f,
            AudioParameterFloatAttributes().withLabel ("dB")));

        return { p.begin(), p.end() };
    }
} // namespace ondaire::param
