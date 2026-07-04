#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "OndaireEngine.h"
#include "OndaireParams.h"

class OndaireAudioProcessor : public juce::AudioProcessor
{
public:
    OndaireAudioProcessor();
    ~OndaireAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                       { return true; }

    const juce::String getName() const override           { return "Ondaire"; }
    bool acceptsMidi() const override                     { return true; }
    bool producesMidi() const override                    { return false; }
    bool isMidiEffect() const override                    { return false; }
    double getTailLengthSeconds() const override          { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override                      { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    ondaire::Snapshot makeSnapshot() const;
    float boolValue (const char* id) const;
    float floatValue (const char* id) const;
    void setParam (const char* id, float plainValue);

    ondaire::Engine engine;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OndaireAudioProcessor)
};
