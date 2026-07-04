#include "PluginEditor.h"
#include "OndairePresets.h"

namespace p = ondaire::param;

//==============================================================================
const juce::Colour OndaireLookAndFeel::background { 0xff191512 };
const juce::Colour OndaireLookAndFeel::panel      { 0xff262019 };
const juce::Colour OndaireLookAndFeel::panelLight { 0xff322a20 };
const juce::Colour OndaireLookAndFeel::cream      { 0xffe8ddc4 };
const juce::Colour OndaireLookAndFeel::brass      { 0xffc8a04b };
const juce::Colour OndaireLookAndFeel::accent     { 0xffb5502e };

OndaireLookAndFeel::OndaireLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Slider::textBoxTextColourId, cream.withAlpha (0.8f));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, cream);
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

    g.setColour (on ? brass : cream.withAlpha (0.75f));
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

    processor.addListener (this);

    setSize (1080, 660);
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
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 14);
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (juce::FontOptions (12.0f)));
    k.label.setColour (juce::Label::textColourId, OndaireLookAndFeel::cream.withAlpha (0.85f));
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
    g.fillAll (OndaireLookAndFeel::background);

    auto drawSection = [&g] (juce::Rectangle<int> r, const juce::String& title)
    {
        if (r.isEmpty())
            return;
        auto rf = r.toFloat();
        g.setColour (OndaireLookAndFeel::panel);
        g.fillRoundedRectangle (rf, 8.0f);
        g.setColour (OndaireLookAndFeel::brass.withAlpha (0.25f));
        g.drawRoundedRectangle (rf.reduced (0.5f), 8.0f, 1.0f);
        g.setColour (OndaireLookAndFeel::brass);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (title.toUpperCase(), r.reduced (12, 6).removeFromTop (16),
                    juce::Justification::centredLeft);
    };

    drawSection (leverPanel,  "Timbre Levers");
    drawSection (oscPanel,    "Oscillator & Register");
    drawSection (filterPanel, "Filter");
    drawSection (envPanel,    "Envelope");
    drawSection (modPanel,    "Vibrato & Tremolo");
    drawSection (outPanel,    "Voice & Output");

    // Wordmark
    auto header = getLocalBounds().removeFromTop (52).reduced (18, 6);
    g.setColour (OndaireLookAndFeel::cream);
    g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    g.drawText ("ONDAIRE", header, juce::Justification::centredLeft);
    g.setColour (OndaireLookAndFeel::brass.withAlpha (0.8f));
    g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::italic)));
    g.drawText ("an Ondioline-inspired electronic instrument",
                header.withTrimmedLeft (170), juce::Justification::centredLeft);
}

void OndaireAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop (52).reduced (18, 12);
    presetBox.setBounds (header.removeFromRight (240));

    keyboard.setBounds (area.removeFromBottom (86).reduced (18, 8));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);

    area.reduce (12, 4);
    const int gap = 8;

    // Top row: lever bank + oscillator section.
    auto topRow = area.removeFromTop (juce::jmax (150, area.getHeight() * 38 / 100));
    leverPanel = topRow.removeFromLeft (topRow.getWidth() * 58 / 100).reduced (gap / 2);
    oscPanel   = topRow.reduced (gap / 2);

    auto placeLevers = [this] (juce::Rectangle<int> r)
    {
        r = r.reduced (12).withTrimmedTop (16);
        const int w = r.getWidth() / numLevers;
        for (auto& l : levers)
            l.button.setBounds (r.removeFromLeft (w));
    };
    placeLevers (leverPanel);

    // Expects a rectangle already trimmed of its section padding/title.
    auto placeKnobs = [] (juce::Rectangle<int> r, std::initializer_list<Knob*> knobs)
    {
        const int w = r.getWidth() / (int) knobs.size();
        for (auto* k : knobs)
        {
            auto cell = r.removeFromLeft (w);
            k->label.setBounds (cell.removeFromTop (16));
            k->slider.setBounds (cell);
        }
    };

    {
        auto r = oscPanel.reduced (12).withTrimmedTop (16);
        auto comboRow = r.removeFromTop (26);
        registerBox.setBounds (comboRow.removeFromLeft (juce::jmin (150, comboRow.getWidth())));
        placeKnobs (r, { &tune, &topsWidth, &drive, &souffle, &percDecay });
    }

    // Bottom row: filter / envelope / modulation / voice+output.
    auto bottomRow = area.reduced (0, gap / 2);
    const int colW = bottomRow.getWidth() / 4;
    filterPanel = bottomRow.removeFromLeft (colW).reduced (gap / 2);
    envPanel    = bottomRow.removeFromLeft (colW).reduced (gap / 2);
    modPanel    = bottomRow.removeFromLeft (colW).reduced (gap / 2);
    outPanel    = bottomRow.reduced (gap / 2);

    {
        auto r = filterPanel.reduced (12).withTrimmedTop (16);
        filterBox.setBounds (r.removeFromTop (26).reduced (4, 0));
        placeKnobs (r, { &cutoff, &resonance });
    }
    placeKnobs (envPanel.reduced (12).withTrimmedTop (16),
                { &attack, &decay, &sustain, &release });
    placeKnobs (modPanel.reduced (12).withTrimmedTop (16),
                { &vibRate, &vibDepth, &tremRate, &tremDepth });
    {
        auto r = outPanel.reduced (12).withTrimmedTop (16);
        modeBox.setBounds (r.removeFromTop (26).reduced (4, 0));
        placeKnobs (r, { &glide, &pbRange, &expression, &gain });
    }
}
