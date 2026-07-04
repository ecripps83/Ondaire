#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "OndairePresets.h"

namespace p = ondaire::param;

OndaireAudioProcessor::OndaireAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OndaireState", p::createLayout())
{
}

void OndaireAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

void OndaireAudioProcessor::releaseResources()
{
    engine.reset();
}

bool OndaireAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

float OndaireAudioProcessor::boolValue (const char* id) const
{
    return apvts.getRawParameterValue (id)->load();
}

float OndaireAudioProcessor::floatValue (const char* id) const
{
    return apvts.getRawParameterValue (id)->load();
}

ondaire::Snapshot OndaireAudioProcessor::makeSnapshot() const
{
    ondaire::Snapshot s;

    // Register 1..4 maps to octave shifts -2..+1 around the keyboard middle.
    s.octaveShift = (int) floatValue (p::registre) - 2;
    s.tuneCents   = floatValue (p::tune);
    s.pulseWidth  = floatValue (p::pulseWidth);

    s.levA  = boolValue (p::leverA)  > 0.5f;
    s.levB  = boolValue (p::leverB)  > 0.5f;
    s.levC  = boolValue (p::leverC)  > 0.5f;
    s.levD  = boolValue (p::leverD)  > 0.5f;
    s.levE  = boolValue (p::leverE)  > 0.5f;
    s.levF  = boolValue (p::leverF)  > 0.5f;
    s.levG  = boolValue (p::leverG)  > 0.5f;
    s.levH  = boolValue (p::leverH)  > 0.5f;
    s.levI  = boolValue (p::leverI)  > 0.5f;
    s.levJ  = boolValue (p::leverJ)  > 0.5f;
    s.levK  = boolValue (p::leverK)  > 0.5f;
    s.levM  = boolValue (p::leverM)  > 0.5f;
    s.levP  = boolValue (p::leverP)  > 0.5f;
    s.levV1 = boolValue (p::leverV1) > 0.5f;
    s.levV2 = boolValue (p::leverV2) > 0.5f;
    s.levW  = boolValue (p::leverW)  > 0.5f;

    s.percDecay = floatValue (p::percDecay);
    s.drive     = floatValue (p::drive);
    s.noise     = floatValue (p::noise);

    s.filterType = (int) floatValue (p::filterType);
    s.cutoff     = floatValue (p::cutoff);
    s.resonance  = floatValue (p::resonance);

    s.attack  = floatValue (p::attack);
    s.decay   = floatValue (p::decay);
    s.sustain = floatValue (p::sustain);
    s.release = floatValue (p::release);

    s.mono    = (int) floatValue (p::mode) == 0;
    s.glide   = floatValue (p::glide);
    s.pbRange = floatValue (p::pbRange);

    s.vibRate   = floatValue (p::vibRate);
    s.vibDepth  = floatValue (p::vibDepth);
    s.tremRate  = floatValue (p::tremRate);
    s.tremDepth = floatValue (p::tremDepth);

    s.expression = floatValue (p::expression);
    s.gainLin    = juce::Decibels::decibelsToGain (floatValue (p::gain));

    return s;
}

void OndaireAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);
    engine.process (buffer, midiMessages, makeSnapshot());
}

//==============================================================================
int OndaireAudioProcessor::getNumPrograms()
{
    return ondaire::presets::count;
}

const juce::String OndaireAudioProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, ondaire::presets::count))
        return ondaire::presets::list[index].name;
    return {};
}

void OndaireAudioProcessor::setParam (const char* id, float plainValue)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
}

void OndaireAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, ondaire::presets::count))
        return;

    currentProgram = index;
    const auto& preset = ondaire::presets::list[index];
    const juce::String levers (preset.levers);

    setParam (p::leverA,  levers.containsChar ('A'));
    setParam (p::leverB,  levers.containsChar ('B'));
    setParam (p::leverC,  levers.containsChar ('C'));
    setParam (p::leverD,  levers.containsChar ('D'));
    setParam (p::leverE,  levers.containsChar ('E'));
    setParam (p::leverF,  levers.containsChar ('F'));
    setParam (p::leverG,  levers.containsChar ('G'));
    setParam (p::leverH,  levers.containsChar ('H'));
    setParam (p::leverI,  levers.containsChar ('I'));
    setParam (p::leverJ,  levers.containsChar ('J'));
    setParam (p::leverK,  levers.containsChar ('K'));
    setParam (p::leverM,  levers.containsChar ('M'));
    setParam (p::leverP,  levers.containsChar ('P'));
    setParam (p::leverV1, levers.containsChar ('1'));
    setParam (p::leverV2, levers.containsChar ('2'));
    setParam (p::leverW,  levers.containsChar ('W'));

    setParam (p::registre,  (float) (preset.registre - 1));
    setParam (p::percDecay, preset.percDecay);
    setParam (p::noise,     preset.noise);
    setParam (p::mode,      preset.poly ? 1.0f : 0.0f);

    // Percussive presets do not sustain; give plucked patches a quick release.
    if (levers.containsChar ('P'))
        setParam (p::release, 0.15f);
}

//==============================================================================
void OndaireAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OndaireAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) state.getProperty ("program", 0);
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessorEditor* OndaireAudioProcessor::createEditor()
{
    return new OndaireAudioProcessorEditor (*this);
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OndaireAudioProcessor();
}
