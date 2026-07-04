// Offline smoke test for the Ondaire engine: renders notes through every
// lever configuration used by the factory presets and checks the output is
// audible, finite and decays to silence after release.

#include "../OndaireEngine.h"
#include "../OndairePresets.h"
#include <cstdio>

namespace
{
    int failures = 0;

    void expect (bool condition, const char* what)
    {
        if (! condition)
        {
            std::printf ("FAIL: %s\n", what);
            ++failures;
        }
    }

    ondaire::Snapshot snapshotFor (const ondaire::presets::Preset& preset)
    {
        ondaire::Snapshot s;
        const juce::String levers (preset.levers);
        s.levA  = levers.containsChar ('A');
        s.levB  = levers.containsChar ('B');
        s.levC  = levers.containsChar ('C');
        s.levD  = levers.containsChar ('D');
        s.levE  = levers.containsChar ('E');
        s.levF  = levers.containsChar ('F');
        s.levG  = levers.containsChar ('G');
        s.levH  = levers.containsChar ('H');
        s.levI  = levers.containsChar ('I');
        s.levJ  = levers.containsChar ('J');
        s.levK  = levers.containsChar ('K');
        s.levM  = levers.containsChar ('M');
        s.levP  = levers.containsChar ('P');
        s.levV1 = levers.containsChar ('1');
        s.levV2 = levers.containsChar ('2');
        s.levW  = levers.containsChar ('W');
        s.octaveShift = preset.registre - 3;
        s.percDecay   = preset.percDecay;
        s.noise       = preset.noise;
        s.mono        = ! preset.poly;
        return s;
    }

    struct Stats { float peak = 0.0f; bool finite = true; };

    Stats renderBlocks (ondaire::Engine& engine, const ondaire::Snapshot& s,
                        const juce::MidiBuffer& midi, int numBlocks, int blockSize)
    {
        Stats stats;
        juce::AudioBuffer<float> buffer (2, blockSize);
        for (int b = 0; b < numBlocks; ++b)
        {
            buffer.clear();
            engine.process (buffer, b == 0 ? midi : juce::MidiBuffer(), s);
            for (int i = 0; i < blockSize; ++i)
            {
                const float v = buffer.getSample (0, i);
                if (! std::isfinite (v))
                    stats.finite = false;
                stats.peak = juce::jmax (stats.peak, std::abs (v));
            }
        }
        return stats;
    }
}

int main()
{
    constexpr double sr = 48000.0;
    constexpr int blockSize = 512;
    const int blocksPerSecond = (int) (sr / blockSize);

    for (int i = 0; i < ondaire::presets::count; ++i)
    {
        const auto& preset = ondaire::presets::list[i];
        const auto s = snapshotFor (preset);

        ondaire::Engine engine;
        engine.prepare (sr, blockSize);

        juce::MidiBuffer noteOn;
        noteOn.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        auto held = renderBlocks (engine, s, noteOn, blocksPerSecond / 2, blockSize);

        expect (held.finite, preset.name);
        expect (held.peak > 1.0e-3f, preset.name);
        expect (held.peak < 4.0f, preset.name);

        juce::MidiBuffer noteOff;
        noteOff.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        renderBlocks (engine, s, noteOff, blocksPerSecond * 3, blockSize);

        // After three seconds of release the engine must be silent again.
        auto tail = renderBlocks (engine, s, juce::MidiBuffer(), 4, blockSize);
        expect (tail.finite, preset.name);
        expect (tail.peak < 1.0e-3f, preset.name);

        std::printf ("%-24s held peak %.3f  tail peak %.6f\n",
                     preset.name, held.peak, tail.peak);
    }

    // Poly chord + pitch wheel + mod wheel sanity check.
    {
        ondaire::Snapshot s;
        s.mono = false;
        ondaire::Engine engine;
        engine.prepare (sr, blockSize);

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.9f), 16);
        midi.addEvent (juce::MidiMessage::noteOn (1, 67, 0.9f), 32);
        midi.addEvent (juce::MidiMessage::pitchWheel (1, 12000), 64);
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 96), 96);
        auto chord = renderBlocks (engine, s, midi, blocksPerSecond / 2, blockSize);
        expect (chord.finite, "poly chord");
        expect (chord.peak > 1.0e-2f, "poly chord audible");
    }

    if (failures == 0)
    {
        std::printf ("\nAll engine smoke tests passed.\n");
        return 0;
    }
    std::printf ("\n%d failure(s).\n", failures);
    return 1;
}
