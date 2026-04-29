/*
  ==============================================================================

    PluginEditor.cpp (Phase 44)
    Created: 2026
    Author: OTODESK (Glitch Arch)
    Description: Editor - Added Drive & Tone UI

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor(GlitchBeatsAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1200, 750);
    setLookAndFeel(&mLookAndFeel);

    auto setupSlider = [&](juce::Slider& s, double defVal)
        {
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setDoubleClickReturnValue(true, defVal);
            addAndMakeVisible(s);
        };

    auto& params = audioProcessor.parameters;

    // --- Core ---
    setupSlider(sensitivitySlider, 0.5); sensitivityAttachment = std::make_unique<Attachment>(params, "sensitivity", sensitivitySlider);
    setupSlider(grainSizeSlider, 100.0); grainSizeAttachment = std::make_unique<Attachment>(params, "grainsize", grainSizeSlider);
    setupSlider(chaosSlider, 0.2); chaosAttachment = std::make_unique<Attachment>(params, "chaos", chaosSlider);
    setupSlider(smoothSlider, 0.2); smoothAttachment = std::make_unique<Attachment>(params, "smooth", smoothSlider);
    setupSlider(pitchSlider, 0.0); pitchAttachment = std::make_unique<Attachment>(params, "pitch", pitchSlider);
    setupSlider(mixSlider, 0.5); mixAttachment = std::make_unique<Attachment>(params, "mix", mixSlider);

    setupSlider(densitySlider, 0.0); densityAttachment = std::make_unique<Attachment>(params, "density", densitySlider);
    setupSlider(spreadSlider, 0.5); spreadAttachment = std::make_unique<Attachment>(params, "stereoSpread", spreadSlider);

    directionBox.addItem("Fwd", 1); directionBox.addItem("Rev", 2); directionBox.addItem("Alt", 3); directionBox.addItem("Rnd", 4);
    addAndMakeVisible(directionBox); directionAttachment = std::make_unique<ComboAttachment>(params, "direction", directionBox);

    windowBox.addItem("Tri", 1); windowBox.addItem("Sine", 2); windowBox.addItem("Sqr", 3); windowBox.addItem("Saw", 4);
    addAndMakeVisible(windowBox); windowAttachment = std::make_unique<ComboAttachment>(params, "winShape", windowBox);

    juce::StringArray routeNames = { "EXT SC", "SELF", "SWAP" };
    for (int i = 0; i < 3; ++i) {
        routingButtons[i].setButtonText(routeNames[i]);
        routingButtons[i].setClickingTogglesState(false);
        routingButtons[i].onClick = [this, i] {
            if (auto* param = audioProcessor.parameters.getParameter("routing"))
                param->setValueNotifyingHost((float)i / 2.0f);
            };
        addAndMakeVisible(routingButtons[i]);
    }

    freezeButton.setButtonText("FREEZE"); freezeButton.setClickingTogglesState(true);
    addAndMakeVisible(freezeButton); freezeAttachment = std::make_unique<ButtonAttachment>(params, "freeze", freezeButton);

    setupSlider(scLowCutSlider, 5.0); scLowCutAttachment = std::make_unique<Attachment>(params, "scLowCut", scLowCutSlider);

    // --- Filter ---
    filterBypassButton.setButtonText("BYPASS"); filterBypassButton.setClickingTogglesState(true);
    addAndMakeVisible(filterBypassButton); filterBypassAttachment = std::make_unique<ButtonAttachment>(params, "filterBypass", filterBypassButton);

    filterTypeBox.addItem("LP", 1); filterTypeBox.addItem("HP", 2); filterTypeBox.addItem("BP", 3); filterTypeBox.addItem("Notch", 4);
    addAndMakeVisible(filterTypeBox); filterTypeAttachment = std::make_unique<ComboAttachment>(params, "filterType", filterTypeBox);

    setupSlider(cutoffSlider, 20000.0); cutoffAttachment = std::make_unique<Attachment>(params, "cutoff", cutoffSlider);
    setupSlider(resonanceSlider, 0.0); resonanceAttachment = std::make_unique<Attachment>(params, "resonance", resonanceSlider);

    // --- LFO ---
    for (int i = 0; i < 6; ++i) {
        juce::String prefix = "lfo" + juce::String(i + 1) + "_";

        lfoActiveButtons[i].setButtonText(juce::String(i + 1));
        lfoActiveButtons[i].setClickingTogglesState(true);
        lfoActiveButtons[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        addAndMakeVisible(lfoActiveButtons[i]);
        lfoActiveAttachments[i] = std::make_unique<ButtonAttachment>(params, prefix + "active", lfoActiveButtons[i]);

        lfoWaveBoxes[i].addItem("Sine", 1); lfoWaveBoxes[i].addItem("Tri", 2);
        lfoWaveBoxes[i].addItem("Saw", 3); lfoWaveBoxes[i].addItem("Sqr", 4); lfoWaveBoxes[i].addItem("S&H", 5); lfoWaveBoxes[i].addItem("Noise", 6);
        addAndMakeVisible(lfoWaveBoxes[i]);
        lfoWaveAttachments[i] = std::make_unique<ComboAttachment>(params, prefix + "wave", lfoWaveBoxes[i]);

        lfoSyncButtons[i].setButtonText("Sync"); lfoSyncButtons[i].setClickingTogglesState(true);
        addAndMakeVisible(lfoSyncButtons[i]);
        lfoSyncAttachments[i] = std::make_unique<ButtonAttachment>(params, prefix + "sync", lfoSyncButtons[i]);

        setupSlider(lfoFreqSliders[i], 0.5);
        lfoFreqAttachments[i] = std::make_unique<Attachment>(params, prefix + "freq", lfoFreqSliders[i]);

        lfoFreqSliders[i].textFromValueFunction = [this, i](double value) {
            if (lfoSyncButtons[i].getToggleState()) {
                int step = (int)(value * 8.99f);
                switch (step) {
                case 0: return juce::String("8/1"); case 1: return juce::String("4/1"); case 2: return juce::String("2/1"); case 3: return juce::String("1/1");
                case 4: return juce::String("1/2"); case 5: return juce::String("1/4"); case 6: return juce::String("1/8"); case 7: return juce::String("1/16");
                case 8: return juce::String("1/32"); default: return juce::String("1/4");
                }
            }
            else { float hz = 0.01f * std::pow(3000.0f, (float)value); return juce::String(hz, 2); }
            };

        setupSlider(lfoMinBoxes[i], 0.0); lfoMinAttachments[i] = std::make_unique<Attachment>(params, prefix + "min", lfoMinBoxes[i]);
        setupSlider(lfoMaxBoxes[i], 100.0); lfoMaxAttachments[i] = std::make_unique<Attachment>(params, prefix + "max", lfoMaxBoxes[i]);

        lfoTargetBoxes[i].addItem("Sens", 1); lfoTargetBoxes[i].addItem("Grain", 2); lfoTargetBoxes[i].addItem("Chaos", 3);
        lfoTargetBoxes[i].addItem("Smooth", 4); lfoTargetBoxes[i].addItem("Pitch", 5); lfoTargetBoxes[i].addItem("Cutoff", 6);
        lfoTargetBoxes[i].addItem("Res", 7); lfoTargetBoxes[i].addItem("Mix", 8); lfoTargetBoxes[i].addItem("Gain", 9);
        lfoTargetBoxes[i].addItem("X-Mix", 10); lfoTargetBoxes[i].addItem("X-P1", 11); lfoTargetBoxes[i].addItem("X-P2", 12);
        lfoTargetBoxes[i].addItem("Crush", 13); lfoTargetBoxes[i].addItem("SeqRate", 14); lfoTargetBoxes[i].addItem("StutDiv", 15); lfoTargetBoxes[i].addItem("FiltBy", 16);
        lfoTargetBoxes[i].addItem("ResTime", 17); lfoTargetBoxes[i].addItem("ResFb", 18);
        lfoTargetBoxes[i].addItem("Drive", 19); lfoTargetBoxes[i].addItem("Tone", 20); // [New]

        addAndMakeVisible(lfoTargetBoxes[i]); lfoTargetAttachments[i] = std::make_unique<ComboAttachment>(params, prefix + "target", lfoTargetBoxes[i]);
    }

    // --- X-Mod ---
    xModModeBox.addItem("Spectral", 1); xModModeBox.addItem("AM (Ring)", 2); xModModeBox.addItem("FM (Crush)", 3);
    addAndMakeVisible(xModModeBox); xModModeAttachment = std::make_unique<ComboAttachment>(params, "xModMode", xModModeBox);
    setupSlider(xModMixSlider, 0.0); xModMixAttachment = std::make_unique<Attachment>(params, "xModMix", xModMixSlider);
    setupSlider(xModParamASlider, 0.5); xModParamAAttachment = std::make_unique<Attachment>(params, "xModParamA", xModParamASlider);
    setupSlider(xModParamBSlider, 0.1); xModParamBAttachment = std::make_unique<Attachment>(params, "xModParamB", xModParamBSlider);

    // --- Sequencer ---
    for (int i = 0; i < 16; ++i) {
        seqStepButtons[static_cast<size_t>(i)].setButtonText(juce::String(i + 1));
        seqStepButtons[static_cast<size_t>(i)].onClick = [this, i] {
            if (auto* p = audioProcessor.parameters.getParameter("seqStep" + juce::String(i))) {
                float val = p->getValue();
                int currentInt = std::round(val * 4.0f);
                int nextInt = (currentInt + 1) % 5;
                p->setValueNotifyingHost((float)nextInt / 4.0f);
                repaint();
            }
            };
        addAndMakeVisible(seqStepButtons[static_cast<size_t>(i)]);
    }

    seqLenDecButton.setButtonText("<");
    seqLenDecButton.onClick = [this] {
        if (auto* p = audioProcessor.parameters.getParameter("seqLength")) {
            float range = p->getNormalisableRange().end - p->getNormalisableRange().start;
            float current = p->getValue() * range + p->getNormalisableRange().start;
            float next = std::max(2.0f, current - 1.0f);
            p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(next));
        }
        };
    addAndMakeVisible(seqLenDecButton);

    seqLenIncButton.setButtonText(">");
    seqLenIncButton.onClick = [this] {
        if (auto* p = audioProcessor.parameters.getParameter("seqLength")) {
            float range = p->getNormalisableRange().end - p->getNormalisableRange().start;
            float current = p->getValue() * range + p->getNormalisableRange().start;
            float next = std::min(16.0f, current + 1.0f);
            p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(next));
        }
        };
    addAndMakeVisible(seqLenIncButton);

    addAndMakeVisible(seqLenLabel);
    seqLenLabel.setJustificationType(juce::Justification::centred);
    seqLenLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    seqRateBox.addItem("1/1", 1); seqRateBox.addItem("1/2", 2); seqRateBox.addItem("1/4", 3);
    seqRateBox.addItem("1/8", 4); seqRateBox.addItem("1/16", 5); seqRateBox.addItem("1/32", 6); seqRateBox.addItem("1/64", 7);
    addAndMakeVisible(seqRateBox); seqRateAttachment = std::make_unique<ComboAttachment>(params, "seqRate", seqRateBox);

    seqBypassButton.setButtonText("BYPASS"); seqBypassButton.setClickingTogglesState(true);
    addAndMakeVisible(seqBypassButton); seqBypassAttachment = std::make_unique<ButtonAttachment>(params, "seqBypass", seqBypassButton);

    // --- Master ---
    setupSlider(masterGainSlider, 0.5); masterGainAttachment = std::make_unique<Attachment>(params, "masterGain", masterGainSlider);
    setupSlider(masterMixSlider, 1.0); masterMixAttachment = std::make_unique<Attachment>(params, "masterMix", masterMixSlider);
    setupSlider(masterCrushSlider, 0.0); masterCrushAttachment = std::make_unique<Attachment>(params, "masterCrush", masterCrushSlider);

    setupSlider(masterDriveSlider, 0.0); masterDriveAttachment = std::make_unique<Attachment>(params, "masterDrive", masterDriveSlider); // [New]
    setupSlider(masterToneSlider, 0.5); masterToneAttachment = std::make_unique<Attachment>(params, "masterTone", masterToneSlider);   // [New]

    setupSlider(masterResoTimeSlider, 0.0); masterResoTimeAttachment = std::make_unique<Attachment>(params, "masterResoTime", masterResoTimeSlider);
    setupSlider(masterResoFdbkSlider, 0.0); masterResoFdbkAttachment = std::make_unique<Attachment>(params, "masterResoFdbk", masterResoFdbkSlider);

    masterStutterButton.setButtonText("STUTTER"); masterStutterButton.setClickingTogglesState(true);
    addAndMakeVisible(masterStutterButton); masterStutterAttachment = std::make_unique<ButtonAttachment>(params, "masterStutterOn", masterStutterButton);

    masterStutterDivBox.addItem("1/4", 1); masterStutterDivBox.addItem("1/8", 2); masterStutterDivBox.addItem("1/16", 3);
    masterStutterDivBox.addItem("1/32", 4); masterStutterDivBox.addItem("1/64", 5);
    addAndMakeVisible(masterStutterDivBox); masterStutterDivAttachment = std::make_unique<ComboAttachment>(params, "masterStutterDiv", masterStutterDivBox);

    // --- Header Buttons ---
    resetButton.setButtonText("RESET"); resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::indianred);
    resetButton.onClick = [this] {
        juce::AlertWindow::showAsync(juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::WarningIcon)
            .withTitle("Reset Parameters")
            .withMessage("Are you sure you want to reset all parameters?")
            .withButton("Yes")
            .withButton("No"),
            [this](int result) { if (result == 1) audioProcessor.resetParameters(); });
        };
    addAndMakeVisible(resetButton);

    randomizeButton.setButtonText("RND");
    randomizeButton.onClick = [this] { audioProcessor.randomizeParameters(true); };
    addAndMakeVisible(randomizeButton);

    undoButton.setButtonText("UNDO"); undoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa0a0a0));
    undoButton.onClick = [this] { audioProcessor.getUndoManager().undo(); };
    addAndMakeVisible(undoButton);

    redoButton.setButtonText("REDO"); redoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa0a0a0));
    redoButton.onClick = [this] { audioProcessor.getUndoManager().redo(); };
    addAndMakeVisible(redoButton);

    loadButton.setButtonText("LOAD"); loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff80b0ff));
    loadButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>("Load Preset", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.xml");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file.existsAsFile()) {
                    std::unique_ptr<juce::XmlElement> xmlState(juce::XmlDocument::parse(file));
                    if (xmlState != nullptr) audioProcessor.parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
                }
            });
        };
    addAndMakeVisible(loadButton);

    saveButton.setButtonText("SAVE"); saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff80b0ff));
    saveButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>("Save Preset", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.xml");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file != juce::File{}) {
                    auto state = audioProcessor.parameters.copyState();
                    std::unique_ptr<juce::XmlElement> xml(state.createXml());
                    xml->writeTo(file);
                }
            });
        };
    addAndMakeVisible(saveButton);

    // [Phase 42] Vis Toggle
    visModeButton.setButtonText("MODE");
    visModeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    visModeButton.onClick = [this] { mVisMode = 1 - mVisMode; repaint(); };
    addAndMakeVisible(visModeButton);

    startTimer(16);
}

PluginEditor::~PluginEditor() { setLookAndFeel(nullptr); stopTimer(); }

void PluginEditor::timerCallback() {
    float targetRMS = audioProcessor.mVisualSidechainRMS.load(); mCurrentSidechainRMS = mCurrentSidechainRMS * 0.8f + targetRMS * 0.2f;
    if (audioProcessor.mVisualTriggerFired.exchange(false)) mTriggerLightDecay = 1.0f; else { mTriggerLightDecay *= 0.90f; if (mTriggerLightDecay < 0.01f) mTriggerLightDecay = 0.0f; }

    if (auto* p = audioProcessor.parameters.getParameter("routing")) {
        int r = std::round(p->getValue() * 2.0f);
        for (int i = 0; i < 3; ++i) routingButtons[i].setToggleState(i == r, juce::dontSendNotification);
    }

    for (int i = 0; i < 6; ++i) {
        bool s = lfoSyncButtons[i].getToggleState();
        if (s != mLastLFOSyncState[i]) { lfoFreqSliders[i].updateText(); mLastLFOSyncState[i] = s; }
    }

    int currentXMode = xModModeBox.getSelectedId();
    if (currentXMode != mLastXModMode) { mLastXModMode = currentXMode; repaint(); }

    mCurrentSeqStep = audioProcessor.mVisualSeqStep.load();

    int currentLen = 16;
    if (auto* p = audioProcessor.parameters.getParameter("seqLength")) {
        float range = p->getNormalisableRange().end - p->getNormalisableRange().start;
        currentLen = std::round(p->getValue() * range + p->getNormalisableRange().start);
        seqLenLabel.setText(juce::String(currentLen), juce::dontSendNotification);
    }

    for (int i = 0; i < 16; ++i) {
        if (auto* p = audioProcessor.parameters.getParameter("seqStep" + juce::String(i))) {
            bool isActive = (i < currentLen);
            seqStepButtons[static_cast<size_t>(i)].setEnabled(isActive);

            if (isActive) {
                int val = std::round(p->getValue() * 4.0f);
                juce::Colour c = juce::Colours::lightgrey;
                if (val == 1) c = juce::Colours::orange;
                else if (val == 2) c = juce::Colours::cyan;
                else if (val == 3) c = juce::Colours::white;
                else if (val == 4) c = juce::Colours::red;
                seqStepButtons[static_cast<size_t>(i)].setColour(juce::TextButton::buttonColourId, c);
                seqStepButtons[static_cast<size_t>(i)].setColour(juce::TextButton::textColourOffId, (val == 0) ? juce::Colours::darkgrey : juce::Colours::black);
            }
            else {
                seqStepButtons[static_cast<size_t>(i)].setColour(juce::TextButton::buttonColourId, juce::Colour(0xffd0d0d0));
                seqStepButtons[static_cast<size_t>(i)].setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
            }
        }
    }

    repaint();
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffdcdcdc));

    juce::Rectangle<int> headerArea(0, 0, getWidth(), 60);
    g.setColour(juce::Colour(0xffc0c0c0)); g.fillRect(headerArea);
    g.setColour(juce::Colours::black); g.setFont(24.0f);
    g.drawText("GlitchNexus", 20, 0, 150, 60, juce::Justification::centredLeft);

    int trigX = 180; int trigY = 20; int trigSize = 20;
    g.setColour(juce::Colours::black.interpolatedWith(juce::Colours::red, mTriggerLightDecay));
    g.fillRect(trigX, trigY, trigSize, trigSize);
    g.setColour(juce::Colours::black); g.drawRect(trigX, trigY, trigSize, trigSize);

    int meterX = 210; int meterW = 150; int meterH = 12; int meterY = 24;
    g.setColour(juce::Colours::white); g.fillRect(meterX, meterY, meterW, meterH);
    float meterLevel = std::clamp(mCurrentSidechainRMS * 4.0f, 0.0f, 1.0f);
    g.setColour(juce::Colours::lightgreen); g.fillRect(meterX, meterY, (int)(meterW * meterLevel), meterH);
    g.setColour(juce::Colours::grey); g.drawRect(meterX, meterY, meterW, meterH);

    g.setColour(juce::Colours::darkgrey); g.setFont(12.0f);
    g.drawText("HPF", 380, 20, 30, 20, juce::Justification::right);

    auto drawSection = [&](juce::String title, int x, int y, int w, int h) {
        g.setColour(juce::Colour(0xfff5f5f5));
        g.fillRoundedRectangle((float)x, (float)y, (float)w, (float)h, 6.0f);
        g.setColour(juce::Colour(0xffb0b0b0));
        g.drawRoundedRectangle((float)x, (float)y, (float)w, (float)h, 6.0f, 1.0f);
        g.setColour(juce::Colours::black); g.setFont(16.0f);
        g.drawText(title, x + 10, y + 5, w - 20, 20, juce::Justification::left);
        };

    int col1X = 20; int col1W = 220;
    int col2X = 260; int col2W = 660;
    int col3X = 940; int col3W = 240;

    int coreY = 80; int coreH = 315;
    int xModY = 410; int xModH = 180;
    int lfoY = 80; int lfoH = 370;
    int visY = 460; int visH = 120;
    int filtY = 80; int filtH = 180;
    int mastY = 280; int mastH = 300;
    int seqY = 600; int seqH = 130;

    drawSection("GLITCH CORE", col1X, coreY, col1W, coreH);
    drawSection("X-MOD", col1X, xModY, col1W, xModH);
    drawSection("LFOs", col2X, lfoY, col2W, lfoH);
    drawSection("FILTER", col3X, filtY, col3W, filtH);
    drawSection("MASTER", col3X, mastY, col3W, mastH);

    // VISUALIZER
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle((float)col2X, (float)visY, (float)col2W, (float)visH, 6.0f);

    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(col2X, visY, col2W, visH);

        if (mVisMode == 0) // Ghost Horizon
        {
            auto& dry = audioProcessor.mVisDryL;
            auto& wet = audioProcessor.mVisWetL;
            juce::Path pDry, pWet;
            float midY = visY + visH * 0.5f;
            float xScale = (float)col2W / (float)dry.size();
            float yScale = visH * 0.45f;

            pDry.startNewSubPath(col2X, midY);
            pWet.startNewSubPath(col2X, midY);

            for (size_t i = 0; i < dry.size(); ++i) {
                float x = col2X + i * xScale;
                pDry.lineTo(x, midY - dry[i] * yScale);
                pWet.lineTo(x, midY - wet[i] * yScale);
            }

            g.setColour(juce::Colours::darkgrey);
            g.strokePath(pDry, juce::PathStrokeType(1.5f));
            g.setColour(juce::Colours::orange);
            g.strokePath(pWet, juce::PathStrokeType(2.0f));
        }
        else // Grain Scanner
        {
            g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
            g.drawHorizontalLine(visY + visH / 2, (float)col2X, (float)(col2X + col2W));

            const auto& grains = audioProcessor.getGlitchCore().getGrains();
            float bufSize = (float)GlitchConfig::RING_BUFFER_SIZE;

            for (const auto& grain : grains) {
                if (grain.isActive) {
                    float gx = col2X + (grain.position / bufSize) * col2W;
                    if (gx > col2X + col2W) gx -= col2W;

                    float height = grain.volume * visH * 0.8f;
                    float y = visY + (visH - height) * 0.5f;

                    g.setColour(juce::Colours::cyan.withAlpha(0.8f));
                    g.fillRect(gx, y, 2.0f, height);
                }
            }
        }
    }

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(12.0f);
    g.drawText(mVisMode == 0 ? "GHOST HORIZON" : "GRAIN SCANNER", col2X + 10, visY + 5, 150, 15, juce::Justification::left);

    g.setColour(juce::Colour(0xffe0e0e0));
    g.fillRoundedRectangle(20, seqY, getWidth() - 40, seqH, 6.0f);
    g.setColour(juce::Colours::black); g.setFont(16.0f);
    g.drawText("SEQUENCER", 30, seqY + 5, 200, 20, juce::Justification::left);

    g.setFont(14.0f); g.setColour(juce::Colours::darkgrey);

    int cy = coreY + 35; int cx = col1X + 10;
    g.drawText("Sens", cx, cy, 50, 20, juce::Justification::left);
    g.drawText("Grain", cx, cy + 30, 50, 20, juce::Justification::left);
    g.drawText("Chaos", cx, cy + 60, 50, 20, juce::Justification::left);
    g.drawText("Smooth", cx, cy + 90, 50, 20, juce::Justification::left);
    g.drawText("Pitch", cx, cy + 120, 50, 20, juce::Justification::left);
    g.drawText("Mix", cx, cy + 150, 50, 20, juce::Justification::left);
    g.drawText("Dens", cx, cy + 180, 50, 20, juce::Justification::left);
    g.drawText("Sprd", cx, cy + 210, 50, 20, juce::Justification::left);

    int xmy = xModY + 35;
    g.drawText("Mode", cx, xmy, 50, 20, juce::Justification::left);
    g.drawText("Mix", cx, xmy + 30, 50, 20, juce::Justification::left);

    juce::String p1 = "P1", p2 = "P2";
    int mode = xModModeBox.getSelectedId();
    if (mode == 1) { p1 = "Drive"; p2 = "Fold"; }
    else if (mode == 2) { p1 = "Drive"; p2 = "Tone"; }
    else if (mode == 3) { p1 = "Depth"; p2 = "Fdbk"; }
    g.drawText(p1, cx, xmy + 60, 50, 20, juce::Justification::left);
    g.drawText(p2, cx, xmy + 90, 50, 20, juce::Justification::left);

    int fy = filtY + 35; int fx = col3X + 10;
    g.drawText("Type", fx, fy + 30, 50, 20, juce::Justification::left);
    g.drawText("Cut", fx, fy + 60, 50, 20, juce::Justification::left);
    g.drawText("Res", fx, fy + 90, 50, 20, juce::Justification::left);

    int my = mastY + 35; int mx = col3X + 10;
    g.drawText("Mix", mx, my, 50, 20, juce::Justification::left);
    g.drawText("Gain", mx, my + 30, 50, 20, juce::Justification::left);
    g.drawText("Crush", mx, my + 60, 50, 20, juce::Justification::left);
    g.drawText("Drive", mx, my + 90, 50, 20, juce::Justification::left);
    g.drawText("Tone", mx, my + 120, 50, 20, juce::Justification::left);
    g.drawText("Time", mx, my + 150, 50, 20, juce::Justification::left);
    g.drawText("Fdbk", mx, my + 180, 50, 20, juce::Justification::left);
    g.drawText("Stut", mx, my + 210, 50, 20, juce::Justification::left);

    int legX = getWidth() - 320; int legY = seqY + 5;
    g.setFont(12.0f);
    auto drawLeg = [&](int x, juce::Colour c, juce::String t) {
        g.setColour(c); g.fillRect(x, legY, 15, 15); g.setColour(juce::Colours::black); g.drawRect(x, legY, 15, 15);
        g.drawText(t, x + 20, legY, 40, 15, juce::Justification::left);
        };
    drawLeg(legX, juce::Colours::lightgrey, "Mute");
    drawLeg(legX + 60, juce::Colours::orange, "Trig");
    drawLeg(legX + 120, juce::Colours::cyan, "Frz");
    drawLeg(legX + 180, juce::Colours::white, "Rnd");
    drawLeg(legX + 240, juce::Colours::red, "Stut");

    for (int i = 0; i < 6; ++i) {
        int r = i / 2; int c = i % 2;
        int lx = col2X + 10 + c * (310 + 10);
        int ly = lfoY + 35 + r * (110 + 10);
        g.drawText("Min", lx + 40, ly + 60 - 12, 30, 12, juce::Justification::left);
        g.drawText("Max", lx + 175, ly + 60 - 12, 30, 12, juce::Justification::left);
    }

    float btnW = (getWidth() - 60.0f) / 16.0f;
    float startX = 30.0f;
    float stepX = startX + mCurrentSeqStep * btnW;
    g.setColour(juce::Colours::red);
    g.drawRoundedRectangle(stepX - 2.0f, seqY + 40.0f - 2.0f, btnW, 74.0f, 6.0f, 3.0f);
}

void PluginEditor::resized()
{
    int col1X = 20; int col1W = 220;
    int col2X = 260;
    int col3X = 940; int col3W = 240;

    int coreY = 80;
    int xModY = 410;
    int lfoY = 80;
    int filtY = 80;
    int mastY = 280;
    int seqY = 600;

    scLowCutSlider.setBounds(415, 20, 150, 20);
    routingButtons[0].setBounds(580, 15, 80, 30);
    routingButtons[1].setBounds(670, 15, 80, 30);
    routingButtons[2].setBounds(760, 15, 80, 30);

    int headRight = getWidth() - 20;
    saveButton.setBounds(headRight - 50, 10, 50, 20);
    loadButton.setBounds(headRight - 105, 10, 50, 20);
    redoButton.setBounds(headRight - 160, 10, 50, 20);
    undoButton.setBounds(headRight - 215, 10, 50, 20);
    resetButton.setBounds(headRight - 80, 35, 80, 20);
    randomizeButton.setBounds(headRight - 165, 35, 80, 20);
    freezeButton.setBounds(headRight - 270, 35, 100, 20);

    visModeButton.setBounds(col2X + 660 - 60, 460 + 5, 50, 20);

    int cy = coreY + 35; int cx = col1X + 60; int cw = 140; int stepY = 30;
    sensitivitySlider.setBounds(cx, cy, cw, 20); cy += stepY;
    grainSizeSlider.setBounds(cx, cy, cw, 20); cy += stepY;
    chaosSlider.setBounds(cx, cy, cw, 20); cy += stepY;
    smoothSlider.setBounds(cx, cy, cw, 20); cy += stepY;
    pitchSlider.setBounds(cx, cy, cw, 20); cy += stepY;
    mixSlider.setBounds(cx, cy, cw, 20); cy += stepY;
    densitySlider.setBounds(cx, cy, cw, 20); cy += stepY;
    spreadSlider.setBounds(cx, cy, cw, 20);

    directionBox.setBounds(col1X + 10, cy + 30, 95, 20);
    windowBox.setBounds(col1X + 115, cy + 30, 95, 20);

    int xmy = xModY + 35;
    xModModeBox.setBounds(cx, xmy, cw, 20); xmy += stepY;
    xModMixSlider.setBounds(cx, xmy, cw, 20); xmy += stepY;
    xModParamASlider.setBounds(cx, xmy, cw, 20); xmy += stepY;
    xModParamBSlider.setBounds(cx, xmy, cw, 20);

    int lfoColW = 310; int lfoRowH = 110;
    for (int i = 0; i < 6; ++i) {
        int r = i / 2; int c = i % 2;
        int lx = col2X + 10 + c * (lfoColW + 10);
        int ly = lfoY + 35 + r * (lfoRowH + 10);

        lfoActiveButtons[i].setBounds(lx + 10, ly, 25, 20);

        lfoWaveBoxes[i].setBounds(lx + 40, ly, 70, 20);
        lfoSyncButtons[i].setBounds(lx + 120, ly, 40, 20);
        lfoTargetBoxes[i].setBounds(lx + 170, ly, 130, 20);

        lfoFreqSliders[i].setBounds(lx + 40, ly + 30, 260, 20);
        lfoMinBoxes[i].setBounds(lx + 40, ly + 60, 125, 20);
        lfoMaxBoxes[i].setBounds(lx + 175, ly + 60, 125, 20);
    }

    int fy = filtY + 35; int fx = col3X + 60; int fw = 160;
    filterBypassButton.setBounds(fx, fy, fw, 20); fy += stepY;
    filterTypeBox.setBounds(fx, fy, fw, 20); fy += stepY;
    cutoffSlider.setBounds(fx, fy, fw, 20); fy += stepY;
    resonanceSlider.setBounds(fx, fy, fw, 20);

    int my = mastY + 35; int mx = col3X + 60; int mw = 160;
    masterMixSlider.setBounds(mx, my, mw, 20); my += stepY;
    masterGainSlider.setBounds(mx, my, mw, 20); my += stepY;
    masterCrushSlider.setBounds(mx, my, mw, 20); my += stepY;
    masterDriveSlider.setBounds(mx, my, mw, 20); my += stepY; // [New]
    masterToneSlider.setBounds(mx, my, mw, 20); my += stepY;  // [New]
    masterResoTimeSlider.setBounds(mx, my, mw, 20); my += stepY;
    masterResoFdbkSlider.setBounds(mx, my, mw, 20); my += stepY;
    masterStutterButton.setBounds(mx, my, 70, 20);
    masterStutterDivBox.setBounds(mx + 80, my, 80, 20);

    seqBypassButton.setBounds(col2X, seqY + 5, 80, 20);

    seqLenDecButton.setBounds(col2X + 100, seqY + 5, 20, 20);
    seqLenLabel.setBounds(col2X + 125, seqY + 5, 30, 20);
    seqLenIncButton.setBounds(col2X + 160, seqY + 5, 20, 20);

    seqRateBox.setBounds(col2X + 200, seqY + 5, 80, 20);

    float totalW = getWidth() - 60.0f;
    float btnW = totalW / 16.0f;
    float sX = 30.0f;
    float bY = seqY + 40.0f;
    for (int i = 0; i < 16; ++i) {
        seqStepButtons[static_cast<size_t>(i)].setBounds((int)(sX + i * btnW), (int)bY, (int)btnW - 4, 70);
    }
}