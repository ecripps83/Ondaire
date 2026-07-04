#include "OndaireEngine.h"

namespace ondaire
{

//==============================================================================
// PolyBLEP band-limiting for the multivibrator edges.
static inline float polyBlep (float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

static inline float wrap01 (float x)
{
    x -= std::floor (x);
    return x;
}

//==============================================================================
void Voice::prepare (double sampleRate)
{
    sr = sampleRate;
    adsr.setSampleRate (sampleRate);
    hpF.prepare (sampleRate);
    lpC.prepare (sampleRate);
    lpCaps.prepare (sampleRate);
    resG.prepare (sampleRate);
    resH.prepare (sampleRate);
    modern.prepare (sampleRate);
    kill();
}

void Voice::kill()
{
    active = held = false;
    note = -1;
    adsr.reset();
    pluckLevel = burstLevel = 0.0f;
    phase = 0.0f;
    hpF.reset(); lpC.reset(); lpCaps.reset();
    resG.reset(); resH.reset(); modern.reset();
}

void Voice::noteOn (int midiNote, float vel, bool legato)
{
    note = midiNote;
    velocity = juce::jmax (0.05f, vel);
    targetFreq = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
    held = true;

    if (! legato || ! active)
    {
        currentFreq = targetFreq;
        active = true;
        adsr.noteOn();
        pluckLevel = 1.0f;
        burstLevel = 1.0f;
    }
}

void Voice::noteOff (bool allowTailOff)
{
    held = false;
    if (! allowTailOff)
    {
        kill();
        return;
    }
    adsr.noteOff();
}

void Voice::updateFilters (const Snapshot& s)
{
    // Lever F: small series capacitor before the resonators -> high-pass.
    hpF.setCutoff (900.0f);

    // Lever C: "emousse l'impulsion" -> low-pass.
    lpC.setCutoff (750.0f);

    // Capacitor levers with no coil engaged load the line like a tone cap.
    // Capacitance "units" chosen so combinations spread musically.
    const float capUnits = (s.levE ? 3.0f : 0.0f) + (s.levI ? 6.0f : 0.0f)
                         + (s.levJ ? 12.0f : 0.0f) + (s.levK ? 24.0f : 0.0f);
    lpCaps.setCutoff (9000.0f / (1.0f + capUnits * 0.5f));

    // Resonator coils G and H: band-pass "formant" circuits. The engaged
    // capacitors set the resonant frequency: f = f0 / sqrt(Cstray + Ccaps).
    const float cTotal = 1.0f + capUnits;
    resG.set (3400.0f / std::sqrt (cTotal), 9.0f);   // low damping, reedy
    resH.set (1500.0f / std::sqrt (cTotal), 5.0f);   // heavier damping, hornlike

    // Modern multimode filter.
    if (s.filterType != 0)
        modern.set (s.cutoff, 0.5f + s.resonance * 11.5f);
}

void Voice::render (float* out, const float* vibratoCents, int n,
                    const Snapshot& s, float pitchBendSemis, float pressureIn)
{
    if (! active)
        return;

    adsr.setParameters ({ juce::jmax (0.001f, s.attack * (1.6f - velocity * 1.2f)),
                          s.decay, s.sustain, s.release });
    updateFilters (s);

    const float octave      = std::exp2 ((float) s.octaveShift + s.tuneCents / 1200.0f);
    const float pw          = s.levB ? 0.5f : s.pulseWidth;
    const float dcComp      = 2.0f * pw - 1.0f;
    const float glideCoeff  = s.glide <= 0.0005f ? 1.0f
                             : 1.0f - std::exp (-1.0f / ((float) sr * s.glide * 0.25f));
    const float pluckMult   = std::exp (-1.0f / ((float) sr * juce::jmax (0.02f, s.percDecay) * 0.25f));
    const float pluckRelMult= std::exp (-1.0f / ((float) sr * 0.03f));
    const float burstMult   = std::exp (-1.0f / ((float) sr * 0.02f));
    const bool  anyCap      = s.levE || s.levI || s.levJ || s.levK;
    const bool  anyCoil     = s.levG || s.levH;

    // Key pressure shapes loudness, as on the expressive keyboard: velocity
    // stands in for how hard the key is pressed, aftertouch swells it live.
    const float pressGain = juce::jlimit (0.1f, 1.3f,
                                          0.25f + 0.75f * velocity + 0.35f * pressureIn);

    for (int i = 0; i < n; ++i)
    {
        // --- pitch --------------------------------------------------------
        currentFreq += (targetFreq - currentFreq) * glideCoeff;
        const float bendFactor = std::exp2 ((pitchBendSemis + vibratoCents[i] * 0.01f) / 12.0f);
        const float freq = juce::jlimit (8.0f, (float) (sr * 0.45),
                                         currentFreq * octave * bendFactor);
        const float dt = freq / (float) sr;

        phase += dt;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // --- multivibrator: band-limited pulse ---------------------------
        float x = phase < pw ? 1.0f : -1.0f;
        x += polyBlep (phase, dt);
        x -= polyBlep (wrap01 (phase - pw), dt);
        x -= dcComp;

        // --- souffle + string-tap chiff, fed through the timbre network ---
        if (s.noise > 0.0f || (s.levM && burstLevel > 1.0e-4f))
        {
            const float white = rng.nextFloat() * 2.0f - 1.0f;
            x += white * (s.noise * 0.35f + (s.levM ? burstLevel * 0.9f : 0.0f));
        }
        burstLevel *= burstMult;

        // --- preamp pentode (lever A bypasses it) -------------------------
        if (! s.levA && s.drive > 0.0f)
        {
            const float g = 1.0f + s.drive * 6.0f;
            x = std::tanh (x * g) / std::tanh (g);
        }

        // --- legacy tone levers ------------------------------------------
        if (s.levF)
            x = hpF.highPass (x);
        if (s.levC)
            x = lpC.lowPass (x);
        if (anyCap && ! anyCoil)
            x = lpCaps.lowPass (x);

        // --- formant resonators -------------------------------------------
        if (anyCoil)
        {
            float wet = 0.0f;
            if (s.levG) wet += resG.bandPass (x) * 1.4f;
            if (s.levH) wet += resH.bandPass (x) * 1.25f;
            x = x * 0.35f + wet;
        }

        // --- modern multimode filter ---------------------------------------
        if (s.filterType != 0)
        {
            float lp, bp, hp;
            modern.process (x, lp, bp, hp);
            x = s.filterType == 1 ? lp : (s.filterType == 2 ? hp : bp);
        }

        // --- envelope -------------------------------------------------------
        float env;
        if (s.levP)
        {
            pluckLevel *= held ? pluckMult : pluckRelMult;
            env = pluckLevel;
            adsr.getNextSample();   // keep ADSR running for state consistency
        }
        else
        {
            env = adsr.getNextSample();
        }

        out[i] += x * env * pressGain;
    }

    // Voice lifetime bookkeeping.
    if (s.levP)
    {
        if (pluckLevel < 1.0e-4f)
            kill();
    }
    else if (! adsr.isActive())
    {
        kill();
    }
}

//==============================================================================
void Engine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    for (auto& v : voices)
        v.prepare (sampleRate);
    monoBus.setSize (1, maxBlockSize);
    vibratoCents.resize ((size_t) maxBlockSize, 0.0f);
    exprSmooth.reset (sampleRate, 0.02);
    gainSmooth.reset (sampleRate, 0.02);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices)
        v.kill();
    heldNotes.clear();
    heldVels.clear();
    sustained.fill (false);
    sustainDown = false;
    pitchBend = 0.0f;
    modWheel = 0.0f;
    exprCC = 1.0f;
    pressure = 0.0f;
    vibPhase = tremPhase = 0.0f;
}

Voice* Engine::findFreeVoice()
{
    for (auto& v : voices)
        if (! v.isActive())
            return &v;

    // Steal the oldest voice.
    Voice* oldest = &voices[0];
    for (auto& v : voices)
        if (v.getStartTime() < oldest->getStartTime())
            oldest = &v;
    oldest->kill();
    return oldest;
}

void Engine::noteOn (int note, float velocity, const Snapshot& s)
{
    sustained[(size_t) note] = false;

    if (s.mono)
    {
        heldNotes.push_back (note);
        heldVels.push_back (velocity);
        const bool legato = heldNotes.size() > 1 && voices[0].isActive();
        voices[0].noteOn (note, velocity, legato);
        voices[0].setStartTime (++noteCounter);
        return;
    }

    // Poly: retrigger an existing voice on the same note, else take a free one.
    for (auto& v : voices)
    {
        if (v.isActive() && v.getNote() == note)
        {
            v.noteOn (note, velocity, false);
            v.setStartTime (++noteCounter);
            return;
        }
    }
    auto* v = findFreeVoice();
    v->noteOn (note, velocity, false);
    v->setStartTime (++noteCounter);
}

void Engine::noteOff (int note, const Snapshot& s)
{
    if (sustainDown)
    {
        sustained[(size_t) note] = true;
        return;
    }

    if (s.mono)
    {
        for (size_t i = heldNotes.size(); i-- > 0;)
        {
            if (heldNotes[i] == note)
            {
                heldNotes.erase (heldNotes.begin() + (long) i);
                heldVels.erase (heldVels.begin() + (long) i);
            }
        }
        if (heldNotes.empty())
        {
            voices[0].noteOff (true);
        }
        else if (voices[0].getNote() == note || voices[0].getNote() < 0)
        {
            // Fall back to the most recent remaining note, legato.
            voices[0].noteOn (heldNotes.back(), heldVels.back(), true);
        }
        return;
    }

    for (auto& v : voices)
        if (v.isActive() && v.isHeld() && v.getNote() == note)
            v.noteOff (true);
}

void Engine::handleMidiEvent (const juce::MidiMessage& m, const Snapshot& s)
{
    if (m.isNoteOn())
    {
        noteOn (m.getNoteNumber(), m.getFloatVelocity(), s);
    }
    else if (m.isNoteOff())
    {
        noteOff (m.getNoteNumber(), s);
    }
    else if (m.isPitchWheel())
    {
        // The pitch wheel stands in for the Ondioline's laterally swinging
        // keyboard: bend range is configurable, default two semitones.
        pitchBend = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * s.pbRange;
    }
    else if (m.isChannelPressure())
    {
        pressure = (float) m.getChannelPressureValue() / 127.0f;
    }
    else if (m.isAftertouch())
    {
        pressure = (float) m.getAfterTouchValue() / 127.0f;
    }
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const float val = (float) m.getControllerValue() / 127.0f;
        switch (cc)
        {
            case 1:  modWheel = val; break;                     // adds vibrato
            case 2:
            case 11: exprCC = val; break;                       // knee lever
            case 71: cc71 = val * 0.7f; break;                  // adds resonance
            case 74: cc74 = val; break;                         // sweeps cutoff
            case 64:
            {
                const bool down = m.getControllerValue() >= 64;
                if (sustainDown && ! down)
                {
                    sustainDown = false;
                    for (int n = 0; n < 128; ++n)
                        if (sustained[(size_t) n])
                        {
                            sustained[(size_t) n] = false;
                            noteOff (n, s);
                        }
                }
                sustainDown = down;
                break;
            }
            case 120:
            case 123:
                reset();
                break;
            default: break;
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        reset();
    }
}

void Engine::renderSegment (int startSample, int numSamples, const Snapshot& s)
{
    if (numSamples <= 0)
        return;

    auto* bus = monoBus.getWritePointer (0) + startSample;

    // Vibrato: levers V1/V2 set the classic depths, W the speed; the modern
    // depth knob, mod wheel and aftertouch add to it.
    const float vibDepthCents = s.vibDepth
                              + (s.levV1 ? 35.0f : 0.0f)
                              + (s.levV2 ? 14.0f : 0.0f)
                              + modWheel * 50.0f
                              + pressure * 10.0f;
    const float vibRate = s.vibRate * (s.levW ? 1.45f : 1.0f);
    const float vibInc  = vibRate / (float) sr;

    for (int i = 0; i < numSamples; ++i)
    {
        vibratoCents[(size_t) i] = vibDepthCents * std::sin (juce::MathConstants<float>::twoPi * vibPhase);
        vibPhase += vibInc;
        if (vibPhase >= 1.0f)
            vibPhase -= 1.0f;
    }

    for (auto& v : voices)
        v.render (bus, vibratoCents.data(), numSamples, s, pitchBend, pressure);

    // Lever D: the LFO chops the signal at the pentode screen, giving the
    // mandolin / banjo repetition effect. A squared-up sine makes the chop.
    const float tremInc = s.tremRate / (float) sr;
    for (int i = 0; i < numSamples; ++i)
    {
        float g = exprSmooth.getNextValue() * gainSmooth.getNextValue();
        if (s.levD)
        {
            const float lfo = std::sin (juce::MathConstants<float>::twoPi * tremPhase);
            const float chop = 0.5f + 0.5f * std::tanh (lfo * 3.0f) / 0.995f;
            g *= 1.0f - s.tremDepth * (1.0f - chop);
            tremPhase += tremInc;
            if (tremPhase >= 1.0f)
                tremPhase -= 1.0f;
        }
        // A touch of output-stage saturation keeps resonant patches in range.
        bus[i] = std::tanh (bus[i] * g);
    }
}

void Engine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                      const Snapshot& snapshot)
{
    const int numSamples = buffer.getNumSamples();
    monoBus.clear (0, 0, numSamples);

    // Apply controller modulation to the snapshot once per block.
    Snapshot s = snapshot;
    s.cutoff    = effectiveCutoff (snapshot);
    s.resonance = effectiveResonance (snapshot);

    exprSmooth.setTargetValue (s.expression * exprCC);
    gainSmooth.setTargetValue (s.gainLin);

    // Mode switches mid-flight leave stale voices sounding; clear them.
    if (s.mono != wasMono)
    {
        reset();
        wasMono = s.mono;
    }

    int segmentStart = 0;
    for (const auto metadata : midi)
    {
        const int pos = juce::jlimit (0, numSamples, metadata.samplePosition);
        renderSegment (segmentStart, pos - segmentStart, s);
        segmentStart = pos;
        handleMidiEvent (metadata.getMessage(), s);
    }
    renderSegment (segmentStart, numSamples - segmentStart, s);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, monoBus, 0, 0, numSamples);
}

} // namespace ondaire
