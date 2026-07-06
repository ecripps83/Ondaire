#include "PluginEditor.h"
#include "OndairePresets.h"
#include "OndaireAssets.h"

namespace p = ondaire::param;

//==============================================================================
const juce::Colour OndaireLookAndFeel::background { 0xff191512 };
const juce::Colour OndaireLookAndFeel::panel      { 0xff262019 };
const juce::Colour OndaireLookAndFeel::panelLight { 0xff322a20 };
const juce::Colour OndaireLookAndFeel::cream      { 0xffe8ddc4 };
const juce::Colour OndaireLookAndFeel::brass      { 0xffc8a04b };
const juce::Colour OndaireLookAndFeel::accent     { 0xffb5502e };
const juce::Colour OndaireLookAndFeel::ink        { 0xff33261a };

OndaireLookAndFeel::OndaireLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Slider::textBoxTextColourId, ink.withAlpha (0.85f));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, brass.withAlpha (0.4f));
    setColour (juce::Label::textColourId, ink);
    setColour (juce::ComboBox::backgroundColourId, panelLight);
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, brass.withAlpha (0.4f));
    setColour (juce::ComboBox::arrowColourId, brass);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, brass.withAlpha (0.3f));
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, cream);
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff14100c));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, brass.withAlpha (0.7f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, brass.withAlpha (0.3f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0x66000000));
}

void OndaireLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float pos, float startAngle, float endAngle,
                                           juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                      .reduced (4.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto square = bounds.withSizeKeepingCentre (size, size);
    const auto centre = square.getCentre();
    const float radius = size * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Track arc
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius - 2.0f, radius - 2.0f,
                         0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff0e0b08));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Value arc
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius - 2.0f, radius - 2.0f,
                         0.0f, startAngle, angle, true);
    g.setColour (brass);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Knob body
    auto body = square.reduced (size * 0.16f);
    g.setGradientFill (juce::ColourGradient (panelLight.brighter (0.25f), body.getTopLeft(),
                                             panel.darker (0.3f), body.getBottomRight(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colour (0xff0e0b08));
    g.drawEllipse (body, 1.2f);

    // Pointer
    const float pointerLen = body.getWidth() * 0.36f;
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -body.getWidth() * 0.5f + 3.0f, 3.0f, pointerLen, 1.5f);
    g.setColour (cream);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void OndaireLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                           bool highlighted, bool)
{
    // Drawn as one of the Ondioline's timbre levers: a slot with an ivory
    // handle that sits up (off) or is pressed down (on).
    auto bounds = b.getLocalBounds().toFloat().reduced (2.0f);
    auto labelArea = bounds.removeFromBottom (16.0f);
    auto slot = bounds.withSizeKeepingCentre (juce::jmin (14.0f, bounds.getWidth()),
                                              bounds.getHeight());

    g.setColour (juce::Colour (0xff0e0b08));
    g.fillRoundedRectangle (slot, 5.0f);
    g.setColour (panelLight.brighter (highlighted ? 0.3f : 0.1f));
    g.drawRoundedRectangle (slot, 5.0f, 1.0f);

    const bool on = b.getToggleState();
    auto handleArea = slot.reduced (2.0f);
    const float handleH = handleArea.getHeight() * 0.42f;
    auto handle = on ? handleArea.removeFromBottom (handleH)
                     : handleArea.removeFromTop (handleH);

    g.setGradientFill (juce::ColourGradient (on ? brass.brighter (0.2f) : cream,
                                             handle.getTopLeft(),
                                             on ? brass.darker (0.4f) : cream.darker (0.5f),
                                             handle.getBottomLeft(), false));
    g.fillRoundedRectangle (handle, 4.0f);
    g.setColour (juce::Colour (0xff0e0b08));
    g.drawRoundedRectangle (handle, 4.0f, 1.0f);

    g.setColour (on ? brass.darker (0.35f) : ink.withAlpha (0.8f));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText (b.getButtonText(), labelArea, juce::Justification::centred);
}

//==============================================================================
OndaireAudioProcessorEditor::OndaireAudioProcessorEditor (OndaireAudioProcessor& proc)
    : AudioProcessorEditor (&proc),
      processor (proc),
      keyboard (proc.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    // Preset selector -------------------------------------------------------
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != processor.getCurrentProgram())
            processor.setCurrentProgram (idx);
    };
    addAndMakeVisible (presetBox);

    // Legacy lever bank ------------------------------------------------------
    static const std::pair<const char*, const char*> leverDefs[numLevers] =
    {
        { p::leverA,  "A"  }, { p::leverB,  "B"  }, { p::leverC,  "C"  },
        { p::leverD,  "D"  }, { p::leverE,  "E"  }, { p::leverF,  "F"  },
        { p::leverG,  "G"  }, { p::leverH,  "H"  }, { p::leverI,  "I"  },
        { p::leverJ,  "J"  }, { p::leverK,  "K"  }, { p::leverM,  "M"  },
        { p::leverP,  "P"  }, { p::leverV1, "V1" }, { p::leverV2, "V2" },
        { p::leverW,  "W"  },
    };
    for (int i = 0; i < numLevers; ++i)
        addLever (levers[i], leverDefs[i].first, leverDefs[i].second);

    addCombo (registerBox, registerAttachment, p::registre);
    addCombo (filterBox, filterAttachment, p::filterType);
    addCombo (modeBox, modeAttachment, p::mode);

    addKnob (tune,       p::tune,       "Tune");
    addKnob (topsWidth,  p::pulseWidth, "Tops Width");
    addKnob (drive,      p::drive,      "Drive");
    addKnob (souffle,    p::noise,      "Souffle");
    addKnob (percDecay,  p::percDecay,  "Perc Decay");

    addKnob (cutoff,     p::cutoff,     "Cutoff");
    addKnob (resonance,  p::resonance,  "Resonance");

    addKnob (attack,     p::attack,     "Attack");
    addKnob (decay,      p::decay,      "Decay");
    addKnob (sustain,    p::sustain,    "Sustain");
    addKnob (release,    p::release,    "Release");

    addKnob (vibRate,    p::vibRate,    "Vib Rate");
    addKnob (vibDepth,   p::vibDepth,   "Vib Depth");
    addKnob (tremRate,   p::tremRate,   "Trem Rate");
    addKnob (tremDepth,  p::tremDepth,  "Trem Depth");

    addKnob (glide,      p::glide,      "Glide");
    addKnob (pbRange,    p::pbRange,    "Bend Range");

    addKnob (expression, p::expression, "Expression");
    addKnob (gain,       p::gain,       "Gain");

    keyboard.setAvailableRange (36, 96);   // 5 octaves around the Ondioline's span
    keyboard.setOctaveForMiddleC (4);
    addAndMakeVisible (keyboard);

    backgroundImage = juce::ImageCache::getFromMemory (OndaireAssets::background_png,
                                                       OndaireAssets::background_pngSize);

    // The artwork bakes in the knob labels of the four lower panels; hide the
    // corresponding component labels so they are not drawn twice.
    for (auto* k : { &cutoff, &resonance, &attack, &decay, &sustain, &release,
                     &vibRate, &vibDepth, &tremRate, &tremDepth,
                     &glide, &pbRange, &expression, &gain })
        k->label.setVisible (false);

    processor.addListener (this);

    setSize (1170, 756);   // 3/4 of the artwork's native 1560 x 1008
}

OndaireAudioProcessorEditor::~OndaireAudioProcessorEditor()
{
    processor.removeListener (this);
    setLookAndFeel (nullptr);
}

void OndaireAudioProcessorEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < ondaire::presets::count; ++i)
        presetBox.addItem (ondaire::presets::list[i].name, i + 1);
    presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected ("Presets (Tableau III)");
}

void OndaireAudioProcessorEditor::audioProcessorChanged (juce::AudioProcessor*,
                                                         const ChangeDetails& details)
{
    if (details.programChanged)
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer (this)]
        {
            if (safe != nullptr)
                safe->presetBox.setSelectedId (safe->processor.getCurrentProgram() + 1,
                                               juce::dontSendNotification);
        });
    }
}

void OndaireAudioProcessorEditor::addKnob (Knob& k, const char* paramID, const juce::String& text)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 15);
    k.slider.setColour (juce::Slider::textBoxTextColourId, OndaireLookAndFeel::ink);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    k.label.setColour (juce::Label::textColourId, OndaireLookAndFeel::ink);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramID, k.slider);
}

void OndaireAudioProcessorEditor::addLever (Lever& l, const char* paramID, const juce::String& text)
{
    l.button.setButtonText (text);
    addAndMakeVisible (l.button);
    l.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, paramID, l.button);
}

void OndaireAudioProcessorEditor::addCombo (juce::ComboBox& box,
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& attachment,
    const char* paramID)
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (
            processor.apvts.getParameter (paramID)))
    {
        int id = 1;
        for (const auto& name : choice->choices)
            box.addItem (name, id++);
    }
    addAndMakeVisible (box);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, paramID, box);
}

//==============================================================================
void OndaireAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (backgroundImage.isValid())
        g.drawImage (backgroundImage, getLocalBounds().toFloat());
    else
        g.fillAll (OndaireLookAndFeel::background);
}

void OndaireAudioProcessorEditor::resized()
{
    // Positions in the artwork's native 1560 x 1008 pixel space.
    const float sx = (float) getWidth()  / 1560.0f;
    const float sy = (float) getHeight() / 1008.0f;
    auto art = [sx, sy] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * sx, y * sy, w * sx, h * sy).toNearestInt();
    };

    presetBox.setBounds (art (1160.0f, 48.0f, 340.0f, 42.0f));

    // Timbre lever bank panel.
    {
        auto r = art (155.0f, 175.0f, 730.0f, 235.0f);
        const int w = r.getWidth() / numLevers;
        for (auto& l : levers)
            l.button.setBounds (r.removeFromLeft (w));
    }

    auto placeKnobs = [] (juce::Rectangle<int> r, std::initializer_list<Knob*> knobs,
                          bool withLabels)
    {
        const int w = r.getWidth() / (int) knobs.size();
        for (auto* k : knobs)
        {
            auto cell = r.removeFromLeft (w);
            if (withLabels)
                k->label.setBounds (cell.removeFromTop (20));
            k->slider.setBounds (cell);
        }
    };

    // Oscillator & register panel (artwork panel is blank; we draw labels).
    registerBox.setBounds (art (940.0f, 190.0f, 220.0f, 40.0f));
    placeKnobs (art (930.0f, 245.0f, 545.0f, 175.0f),
                { &tune, &topsWidth, &drive, &souffle, &percDecay }, true);

    // Lower panels: the artwork provides the knob labels at y ~578, so the
    // sliders sit directly beneath them.
    filterBox.setBounds (art (130.0f, 508.0f, 225.0f, 38.0f));
    placeKnobs (art (100.0f, 595.0f, 280.0f, 200.0f), { &cutoff, &resonance }, false);

    placeKnobs (art (408.0f, 595.0f, 345.0f, 200.0f),
                { &attack, &decay, &sustain, &release }, false);

    placeKnobs (art (784.0f, 595.0f, 315.0f, 200.0f),
                { &vibRate, &vibDepth, &tremRate, &tremDepth }, false);

    modeBox.setBounds (art (1145.0f, 508.0f, 295.0f, 38.0f));
    placeKnobs (art (1128.0f, 595.0f, 320.0f, 200.0f),
                { &glide, &pbRange, &expression, &gain }, false);

    keyboard.setBounds (art (70.0f, 840.0f, 1420.0f, 130.0f));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);
}
