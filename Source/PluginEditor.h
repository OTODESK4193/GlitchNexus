/*
  ==============================================================================

    PluginEditor.h (Phase 44)
    Created: 2026
    Author: OTODESK (Glitch Arch)
    Description: Editor Header - Added Drive & Tone Sliders

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// --- Custom LookAndFeel ---
struct AbletonLookAndFeel : public juce::LookAndFeel_V4
{
    AbletonLookAndFeel() {
        setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        setColour(juce::Slider::trackColourId, juce::Colours::lightgrey);
        setColour(juce::Slider::backgroundColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
        setColour(juce::ComboBox::arrowColourId, juce::Colours::darkgrey);
        setColour(juce::ComboBox::textColourId, juce::Colours::black);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colours::white);
        setColour(juce::PopupMenu::textColourId, juce::Colours::black);
        setColour(juce::PopupMenu::headerTextColourId, juce::Colours::grey);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colours::orange);
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::TextButton::buttonColourId, juce::Colours::white);
        setColour(juce::TextButton::textColourOffId, juce::Colours::black);
        setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearHorizontal) {
            g.setColour(slider.findColour(juce::Slider::backgroundColourId));
            g.fillRect(x, y, width, height);
            g.setColour(slider.findColour(juce::Slider::thumbColourId).withAlpha(0.7f));
            float valRatio = (float)((slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
            g.fillRect(x, y, (int)(width * valRatio), height);
            g.setColour(juce::Colours::grey);
            g.drawRect(x, y, width, height);
            g.setColour(juce::Colours::black);
            g.setFont(14.0f);
            g.drawText(slider.getTextFromValue(slider.getValue()), x, y, width, height, juce::Justification::centred);
        }
        else {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }
};

class PluginEditor : public juce::AudioProcessorEditor,
    public juce::Timer
{
public:
    explicit PluginEditor(GlitchBeatsAudioProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    GlitchBeatsAudioProcessor& audioProcessor;
    AbletonLookAndFeel mLookAndFeel;

    // --- Core ---
    juce::Slider sensitivitySlider;
    juce::Slider grainSizeSlider;
    juce::Slider chaosSlider;
    juce::Slider smoothSlider;
    juce::Slider pitchSlider;
    juce::Slider mixSlider;
    juce::Slider densitySlider;
    juce::Slider spreadSlider;
    juce::ComboBox directionBox;
    juce::ComboBox windowBox;

    std::array<juce::TextButton, 3> routingButtons;
    juce::Slider scLowCutSlider;
    juce::TextButton freezeButton;

    // --- Filter ---
    juce::TextButton filterBypassButton;
    juce::ComboBox filterTypeBox;
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;

    // --- LFO ---
    std::array<juce::TextButton, 6> lfoActiveButtons;
    std::array<juce::ComboBox, 6> lfoWaveBoxes;
    std::array<juce::TextButton, 6> lfoSyncButtons;
    std::array<juce::Slider, 6> lfoFreqSliders;
    std::array<juce::Slider, 6> lfoMinBoxes;
    std::array<juce::Slider, 6> lfoMaxBoxes;
    std::array<juce::ComboBox, 6> lfoTargetBoxes;

    // --- X-Mod ---
    juce::ComboBox xModModeBox;
    juce::Slider xModMixSlider;
    juce::Slider xModParamASlider;
    juce::Slider xModParamBSlider;

    // --- Sequencer ---
    std::array<juce::TextButton, 16> seqStepButtons;
    juce::TextButton seqLenDecButton;
    juce::TextButton seqLenIncButton;
    juce::Label seqLenLabel;
    juce::ComboBox seqRateBox;
    juce::TextButton seqBypassButton;

    // --- Master ---
    juce::Slider masterMixSlider;
    juce::Slider masterGainSlider;
    juce::Slider masterCrushSlider;
    juce::Slider masterDriveSlider; // [New]
    juce::Slider masterToneSlider;  // [New]
    juce::Slider masterResoTimeSlider;
    juce::Slider masterResoFdbkSlider;
    juce::TextButton masterStutterButton;
    juce::ComboBox masterStutterDivBox;

    // --- Header ---
    juce::TextButton resetButton;
    juce::TextButton randomizeButton;
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton loadButton;
    juce::TextButton saveButton;

    juce::TextButton visModeButton;
    int mVisMode = 0;

    // Attachments
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<Attachment> sensitivityAttachment;
    std::unique_ptr<Attachment> grainSizeAttachment;
    std::unique_ptr<Attachment> chaosAttachment;
    std::unique_ptr<Attachment> mixAttachment;
    std::unique_ptr<Attachment> smoothAttachment;
    std::unique_ptr<Attachment> pitchAttachment;
    std::unique_ptr<Attachment> densityAttachment;
    std::unique_ptr<Attachment> spreadAttachment;
    std::unique_ptr<ComboAttachment> directionAttachment;
    std::unique_ptr<ComboAttachment> windowAttachment;

    std::unique_ptr<ButtonAttachment> freezeAttachment;
    std::unique_ptr<Attachment> scLowCutAttachment;

    std::unique_ptr<ButtonAttachment> filterBypassAttachment;
    std::unique_ptr<ComboAttachment> filterTypeAttachment;
    std::unique_ptr<Attachment> cutoffAttachment;
    std::unique_ptr<Attachment> resonanceAttachment;

    std::array<std::unique_ptr<ButtonAttachment>, 6> lfoActiveAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 6> lfoWaveAttachments;
    std::array<std::unique_ptr<ButtonAttachment>, 6> lfoSyncAttachments;
    std::array<std::unique_ptr<Attachment>, 6> lfoFreqAttachments;
    std::array<std::unique_ptr<Attachment>, 6> lfoMinAttachments;
    std::array<std::unique_ptr<Attachment>, 6> lfoMaxAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 6> lfoTargetAttachments;

    std::unique_ptr<ComboAttachment> xModModeAttachment;
    std::unique_ptr<Attachment> xModMixAttachment;
    std::unique_ptr<Attachment> xModParamAAttachment;
    std::unique_ptr<Attachment> xModParamBAttachment;

    std::unique_ptr<ComboAttachment> seqRateAttachment;
    std::unique_ptr<ButtonAttachment> seqBypassAttachment;

    std::unique_ptr<Attachment> masterMixAttachment;
    std::unique_ptr<Attachment> masterGainAttachment;
    std::unique_ptr<Attachment> masterCrushAttachment;
    std::unique_ptr<Attachment> masterDriveAttachment;
    std::unique_ptr<Attachment> masterToneAttachment;
    std::unique_ptr<Attachment> masterResoTimeAttachment;
    std::unique_ptr<Attachment> masterResoFdbkAttachment;
    std::unique_ptr<ButtonAttachment> masterStutterAttachment;
    std::unique_ptr<ComboAttachment> masterStutterDivAttachment;

    float mCurrentSidechainRMS = 0.0f;
    float mTriggerLightDecay = 0.0f;

    std::array<bool, 6> mLastLFOSyncState{ false };
    int mLastXModMode = 0;
    int mCurrentSeqStep = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};