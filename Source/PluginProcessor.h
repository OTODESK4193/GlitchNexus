/*
  ==============================================================================

    PluginProcessor.h (Phase 44)
    Created: 2026
    Author: OTODESK (Glitch Arch)
    Description: Processor - Added Drive & Tone Params

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory> 
#include <atomic>
#include "GlitchEngine.h"

class GlitchBeatsAudioProcessor : public juce::AudioProcessor
{
public:
    GlitchBeatsAudioProcessor();
    ~GlitchBeatsAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    juce::UndoManager mUndoManager;

    juce::UndoManager& getUndoManager() { return mUndoManager; }

    void randomizeParameters(bool includeSequencer = true);
    void resetParameters();

    std::atomic<float> mVisualSidechainRMS{ 0.0f };
    std::atomic<bool> mVisualTriggerFired{ false };
    std::atomic<int> mVisualSeqStep{ 0 };

    static constexpr int VIS_SIZE = 256;
    std::array<float, VIS_SIZE> mVisDryL;
    std::array<float, VIS_SIZE> mVisWetL;
    std::atomic<int> mVisWritePos{ 0 };

    const GlitchCore& getGlitchCore() const { return mGlitchCore; }

private:
    std::unique_ptr<float[]> mMainRingBufferL;
    std::unique_ptr<float[]> mMainRingBufferR;
    std::unique_ptr<float[]> mSidechainRingBufferL;
    std::unique_ptr<float[]> mSidechainRingBufferR;

    int mWritePos = 0;
    double mSampleRate = 44100.0;

    GlitchCore mGlitchCore;
    TransientDetector mDetector;
    juce::Random mRandom;
    GlitchFilter mFilterL;
    GlitchFilter mFilterR;
    std::array<GlitchLFO, 6> mLFOs;
    SpectralMorph mSpectralL;
    SpectralMorph mSpectralR;
    CrossModulator mCrossModL;
    CrossModulator mCrossModR;
    GlitchSequencer mSequencer;
    MasterFX mMasterFX;
    SimpleHPF mScHpfL;
    SimpleHPF mScHpfR;

    int mLastStep = -1;
    float mRndSens = 0.0f;
    float mRndGrain = 0.0f;
    float mRndChaos = 0.0f;
    float mRndPitch = 0.0f;

    std::atomic<float>* paramSensitivity = nullptr;
    std::atomic<float>* paramGrainSize = nullptr;
    std::atomic<float>* paramChaos = nullptr;
    std::atomic<float>* paramMix = nullptr;
    std::atomic<float>* paramRouting = nullptr;
    std::atomic<float>* paramSmooth = nullptr;
    std::atomic<float>* paramFreeze = nullptr;
    std::atomic<float>* paramPitch = nullptr;
    std::atomic<float>* paramDensity = nullptr;
    std::atomic<float>* paramDirection = nullptr;
    std::atomic<float>* paramWinShape = nullptr;
    std::atomic<float>* paramStereoSpread = nullptr;

    std::atomic<float>* paramFilterType = nullptr;
    std::atomic<float>* paramCutoff = nullptr;
    std::atomic<float>* paramResonance = nullptr;
    std::atomic<float>* paramFilterBypass = nullptr;

    std::atomic<float>* paramMasterGain = nullptr;
    std::atomic<float>* paramMasterMix = nullptr;
    std::atomic<float>* paramMasterCrush = nullptr;
    std::atomic<float>* paramMasterDrive = nullptr; // [New]
    std::atomic<float>* paramMasterTone = nullptr;  // [New]
    std::atomic<float>* paramMasterResoTime = nullptr;
    std::atomic<float>* paramMasterResoFdbk = nullptr;
    std::atomic<float>* paramMasterStutterOn = nullptr;
    std::atomic<float>* paramMasterStutterDiv = nullptr;

    std::atomic<float>* paramSCLowCut = nullptr;

    std::array<std::atomic<float>*, 6> paramLFOWave{ nullptr };
    std::array<std::atomic<float>*, 6> paramLFOSync{ nullptr };
    std::array<std::atomic<float>*, 6> paramLFOFreq{ nullptr };
    std::array<std::atomic<float>*, 6> paramLFOMin{ nullptr };
    std::array<std::atomic<float>*, 6> paramLFOMax{ nullptr };
    std::array<std::atomic<float>*, 6> paramLFOTarget{ nullptr };
    std::array<std::atomic<float>*, 6> paramLFOActive{ nullptr };

    std::atomic<float>* paramXModMode = nullptr;
    std::atomic<float>* paramXModMix = nullptr;
    std::atomic<float>* paramXModParamA = nullptr;
    std::atomic<float>* paramXModParamB = nullptr;

    std::array<std::atomic<float>*, 16> paramSeqSteps{ nullptr };

    std::atomic<float>* paramSeqLength = nullptr;
    std::atomic<float>* paramSeqRate = nullptr;
    std::atomic<float>* paramSeqBypass = nullptr;

    std::atomic<float>* paramTrigA = nullptr;
    std::atomic<float>* paramTrigB = nullptr;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    bool mIsPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchBeatsAudioProcessor)
};