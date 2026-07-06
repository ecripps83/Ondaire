#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

//==============================================================================
// Bakelite-and-brass look, loosely after the Ondioline's cabinet.
class OndaireLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OndaireLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    static const juce::Colour background, panel, panelLight, cream, brass, accent, ink;
};

//==============================================================================
class OndaireAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::AudioProcessorListener
{
public:
    explicit OndaireAudioProcessorEditor (OndaireAudioProcessor&);
    ~OndaireAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override {}
    void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Lever
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    void addKnob (Knob&, const char* paramID, const juce::String& text);
    void addLever (Lever&, const char* paramID, const juce::String& text);
    void addCombo (juce::ComboBox&,
                   std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>&,
                   const char* paramID);
    void refreshPresetBox();

    OndaireAudioProcessor& processor;
    OndaireLookAndFeel lnf;

    juce::ComboBox presetBox;

    // Legacy section
    static constexpr int numLevers = 16;
    Lever levers[numLevers];
    juce::ComboBox registerBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> registerAttachment;
    Knob tune, topsWidth, drive, souffle, percDecay;

    // Modern section
    juce::ComboBox filterBox, modeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterAttachment, modeAttachment;
    Knob cutoff, resonance;
    Knob attack, decay, sustain, release;
    Knob vibRate, vibDepth, tremRate, tremDepth;
    Knob glide, pbRange;
    Knob expression, gain;

    juce::MidiKeyboardComponent keyboard;

    // The cabinet artwork. All control positions are expressed in its native
    // 1560 x 1008 pixel space and scaled to the editor bounds.
    juce::Image backgroundImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OndaireAudioProcessorEditor)
};
