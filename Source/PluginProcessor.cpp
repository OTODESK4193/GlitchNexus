/*
  ==============================================================================

    PluginProcessor.cpp (Phase 44)
    Created: 2026
    Author: OTODESK (Glitch Arch)
    Description: Processor - Drive & Tone Implemented (Safe Random)

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

GlitchBeatsAudioProcessor::GlitchBeatsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
    ),
#endif 
    parameters(*this, &mUndoManager, "PARAMETERS", createParameterLayout())
{
    mRandom.setSeedRandomly();

    paramSensitivity = parameters.getRawParameterValue("sensitivity");
    paramGrainSize = parameters.getRawParameterValue("grainsize");
    paramChaos = parameters.getRawParameterValue("chaos");
    paramMix = parameters.getRawParameterValue("mix");
    paramRouting = parameters.getRawParameterValue("routing");
    paramSmooth = parameters.getRawParameterValue("smooth");
    paramFreeze = parameters.getRawParameterValue("freeze");
    paramPitch = parameters.getRawParameterValue("pitch");

    paramDensity = parameters.getRawParameterValue("density");
    paramDirection = parameters.getRawParameterValue("direction");
    paramWinShape = parameters.getRawParameterValue("winShape");
    paramStereoSpread = parameters.getRawParameterValue("stereoSpread");

    paramFilterType = parameters.getRawParameterValue("filterType");
    paramCutoff = parameters.getRawParameterValue("cutoff");
    paramResonance = parameters.getRawParameterValue("resonance");
    paramFilterBypass = parameters.getRawParameterValue("filterBypass");

    paramMasterGain = parameters.getRawParameterValue("masterGain");
    paramMasterMix = parameters.getRawParameterValue("masterMix");
    paramMasterCrush = parameters.getRawParameterValue("masterCrush");
    paramMasterDrive = parameters.getRawParameterValue("masterDrive"); // [New]
    paramMasterTone = parameters.getRawParameterValue("masterTone");  // [New]
    paramMasterResoTime = parameters.getRawParameterValue("masterResoTime");
    paramMasterResoFdbk = parameters.getRawParameterValue("masterResoFdbk");
    paramMasterStutterOn = parameters.getRawParameterValue("masterStutterOn");
    paramMasterStutterDiv = parameters.getRawParameterValue("masterStutterDiv");

    paramSCLowCut = parameters.getRawParameterValue("scLowCut");

    for (int i = 0; i < 6; ++i) {
        juce::String prefix = "lfo" + juce::String(i + 1) + "_";
        paramLFOWave[i] = parameters.getRawParameterValue(prefix + "wave");
        paramLFOSync[i] = parameters.getRawParameterValue(prefix + "sync");
        paramLFOFreq[i] = parameters.getRawParameterValue(prefix + "freq");
        paramLFOMin[i] = parameters.getRawParameterValue(prefix + "min");
        paramLFOMax[i] = parameters.getRawParameterValue(prefix + "max");
        paramLFOTarget[i] = parameters.getRawParameterValue(prefix + "target");
        paramLFOActive[i] = parameters.getRawParameterValue(prefix + "active");
    }

    paramXModMode = parameters.getRawParameterValue("xModMode");
    paramXModMix = parameters.getRawParameterValue("xModMix");
    paramXModParamA = parameters.getRawParameterValue("xModParamA");
    paramXModParamB = parameters.getRawParameterValue("xModParamB");

    for (int i = 0; i < 16; ++i)
    {
        juce::String id = "seqStep" + juce::String(i);
        paramSeqSteps[static_cast<size_t>(i)] = parameters.getRawParameterValue(id);
    }
    paramSeqLength = parameters.getRawParameterValue("seqLength");
    paramSeqRate = parameters.getRawParameterValue("seqRate");
    paramSeqBypass = parameters.getRawParameterValue("seqBypass");

    paramTrigA = parameters.getRawParameterValue("trigA");
    paramTrigB = parameters.getRawParameterValue("trigB");
}

GlitchBeatsAudioProcessor::~GlitchBeatsAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout GlitchBeatsAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sensitivity", 1), "Sensitivity", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("grainsize", 1), "Grain Size", 10.0f, 500.0f, 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("chaos", 1), "Chaos", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mix", 1), "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("routing", 1), "Routing Mode", juce::StringArray{ "Ext Sidechain", "Self Trigger", "Swap Inputs" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("smooth", 1), "Smoothness", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("freeze", 1), "Freeze", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pitch", 1), "Pitch", -12.0f, 12.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("density", 1), "Density", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("direction", 1), "Direction", juce::StringArray{ "Forward", "Reverse", "Alt", "Random" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("winShape", 1), "Window", juce::StringArray{ "Triangle", "Sine", "Square", "Saw" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("stereoSpread", 1), "Spread", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("filterType", 1), "Filter Type", juce::StringArray{ "Low Pass", "High Pass", "Band Pass", "Notch" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cutoff", 1), "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("resonance", 1), "Resonance", 0.0f, 0.95f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("filterBypass", 1), "Filter Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterGain", 1), "Master Gain", -60.0f, 6.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterMix", 1), "Master Mix", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterCrush", 1), "Master Crush", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterDrive", 1), "Master Drive", 0.0f, 1.0f, 0.0f)); // [New]
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterTone", 1), "Master Tone", 0.0f, 1.0f, 0.5f));   // [New]
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterResoTime", 1), "Reso Time", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterResoFdbk", 1), "Reso Fdbk", 0.0f, 0.95f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("masterStutterOn", 1), "Stutter Active", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("masterStutterDiv", 1), "Stutter Div", juce::StringArray{ "1/4", "1/8", "1/16", "1/32", "1/64" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("scLowCut", 1), "SC HPF", juce::NormalisableRange<float>(5.0f, 1000.0f, 1.0f, 0.5f), 5.0f));

    juce::StringArray lfoTargets = { "Sensitivity", "Grain Size", "Chaos", "Smoothness", "Pitch", "Cutoff", "Resonance", "Mix", "Master Gain", "X-Mod Mix", "X-Mod Param A", "X-Mod Param B", "Master Crush", "Seq Rate", "Stutter Div", "Filter Bypass", "Reso Time", "Reso Fdbk", "Drive", "Tone" };

    for (int i = 1; i <= 6; ++i) {
        juce::String prefix = "lfo" + juce::String(i) + "_";
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(prefix + "wave", 1), "LFO " + juce::String(i) + " Wave", juce::StringArray{ "Sine", "Triangle", "Saw", "Square", "S&H", "Noise" }, 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(prefix + "sync", 1), "LFO " + juce::String(i) + " Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(prefix + "freq", 1), "LFO " + juce::String(i) + " Freq", 0.0f, 1.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(prefix + "min", 1), "LFO " + juce::String(i) + " Min", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(prefix + "max", 1), "LFO " + juce::String(i) + " Max", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(prefix + "target", 1), "LFO " + juce::String(i) + " Target", lfoTargets, 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(prefix + "active", 1), "LFO " + juce::String(i) + " Active", false));
    }

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("xModMode", 1), "X-Mod Mode", juce::StringArray{ "Spectral Vocoder", "AM (Ring Mod)", "FM (Crush)" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("xModMix", 1), "X-Mod Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("xModParamA", 1), "Param A", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("xModParamB", 1), "Param B", 0.0f, 1.0f, 0.1f));

    for (int i = 0; i < 16; ++i) {
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID("seqStep" + juce::String(i), 1), "Seq Step " + juce::String(i + 1), 0, 4, 1));
    }
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID("seqLength", 1), "Seq Length", 2, 16, 16));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("seqRate", 1), "Seq Rate", juce::StringArray{ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" }, 4));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("seqBypass", 1), "Seq Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("trigA", 1), "Trig A (Auto RND)", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("trigB", 1), "Trig B (Auto Freeze)", false));

    return { params.begin(), params.end() };
}

//==============================================================================
void GlitchBeatsAudioProcessor::randomizeParameters(bool includeSequencer)
{
    auto setParamNorm = [&](const juce::String& id, float normValue) { if (auto* p = parameters.getParameter(id)) p->setValueNotifyingHost(normValue); };
    auto setParamWorld = [&](const juce::String& id, float worldValue) { if (auto* p = parameters.getParameter(id)) p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(worldValue)); };

    float grainRnd = mRandom.nextFloat();
    setParamWorld("grainsize", 10.0f + grainRnd * 490.0f);
    setParamNorm("chaos", mRandom.nextFloat());
    setParamNorm("smooth", mRandom.nextFloat());
    setParamNorm("mix", 0.3f + mRandom.nextFloat() * 0.6f);

    int pitchInt = mRandom.nextInt(25) - 12;
    setParamWorld("pitch", (float)pitchInt);

    setParamNorm("density", mRandom.nextFloat());
    setParamNorm("stereoSpread", mRandom.nextFloat());

    if (mRandom.nextFloat() > 0.5f) setParamWorld("direction", (float)mRandom.nextInt(4));
    if (mRandom.nextFloat() > 0.5f) setParamWorld("winShape", (float)mRandom.nextInt(4));

    // X-Mod
    if (mRandom.nextFloat() > 0.3f) {
        setParamWorld("xModMode", (float)mRandom.nextInt(3));
        setParamNorm("xModMix", mRandom.nextFloat() * 0.6f);
        setParamNorm("xModParamA", mRandom.nextFloat());
        setParamNorm("xModParamB", mRandom.nextFloat());
    }

    if (includeSequencer) {
        if (mRandom.nextFloat() > 0.0f) {
            for (int i = 0; i < 16; ++i) {
                float r = mRandom.nextFloat();
                int val = 0;
                if (r < 0.4f) val = 0;
                else if (r < 0.7f) val = 1;
                else if (r < 0.8f) val = 2;
                else if (r < 0.9f) val = 3;
                else val = 4;
                setParamWorld("seqStep" + juce::String(i), (float)val);
            }
        }
    }

    // [Excluded: Crush, Drive, Tone]

    // Master Resonator
    if (mRandom.nextFloat() > 0.5f) {
        setParamNorm("masterResoTime", mRandom.nextFloat() * 0.3f);
        setParamNorm("masterResoFdbk", mRandom.nextFloat() * 0.4f);
    }
    else {
        setParamNorm("masterResoTime", 0.0f);
        setParamNorm("masterResoFdbk", 0.0f);
    }

    setParamWorld("masterStutterDiv", (float)mRandom.nextInt(5));

    // LFOs
    for (int i = 2; i <= 6; ++i) {
        juce::String prefix = "lfo" + juce::String(i) + "_";
        if (mRandom.nextFloat() > 0.6f) {
            setParamWorld(prefix + "wave", (float)mRandom.nextInt(6));
            setParamNorm(prefix + "active", (mRandom.nextFloat() > 0.5f) ? 1.0f : 0.0f);
            setParamNorm(prefix + "freq", mRandom.nextFloat());
            setParamNorm(prefix + "min", mRandom.nextFloat());
            setParamNorm(prefix + "max", mRandom.nextFloat());
            setParamWorld(prefix + "target", (float)mRandom.nextInt(20)); // Expanded range
        }
    }
}

void GlitchBeatsAudioProcessor::resetParameters()
{
    auto resetParam = [&](const juce::String& id, float normValue) { if (auto* p = parameters.getParameter(id)) p->setValueNotifyingHost(normValue); };
    resetParam("sensitivity", 0.5f); resetParam("grainsize", 0.18f); resetParam("chaos", 0.2f); resetParam("mix", 0.5f);
    resetParam("smooth", 0.2f); resetParam("pitch", 0.5f);
    resetParam("density", 0.0f); resetParam("stereoSpread", 0.5f); resetParam("direction", 0.0f); resetParam("winShape", 0.0f);

    resetParam("filterType", 0.0f); resetParam("cutoff", 1.0f);
    resetParam("resonance", 0.0f); resetParam("xModMix", 0.0f);
    resetParam("filterBypass", 0.0f); resetParam("seqBypass", 0.0f);

    resetParam("masterMix", 1.0f);
    resetParam("masterGain", 0.5f);
    resetParam("masterCrush", 0.0f);
    resetParam("masterDrive", 0.0f);
    resetParam("masterTone", 0.5f);
    resetParam("masterResoTime", 0.0f);
    resetParam("masterResoFdbk", 0.0f);
    resetParam("scLowCut", 0.0f);

    for (int i = 0; i < 16; ++i) resetParam("seqStep" + juce::String(i), 0.25f);
    for (int i = 1; i <= 6; ++i) {
        juce::String prefix = "lfo" + juce::String(i) + "_";
        resetParam(prefix + "wave", 0.0f);
        resetParam(prefix + "active", 0.0f);
        resetParam(prefix + "mix", 0.0f);
    }
}

//==============================================================================
bool GlitchBeatsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) { if (!layouts.getMainInputChannelSet().isDisabled()) return false; }
    const auto& sidechain = layouts.inputBuses.size() > 1 ? layouts.inputBuses[1] : juce::AudioChannelSet::disabled();
    if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void GlitchBeatsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    setLatencySamples(GlitchConfig::FFT_SIZE);
    mMainRingBufferL = std::make_unique<float[]>(GlitchConfig::RING_BUFFER_SIZE);
    mMainRingBufferR = std::make_unique<float[]>(GlitchConfig::RING_BUFFER_SIZE);
    mSidechainRingBufferL = std::make_unique<float[]>(GlitchConfig::RING_BUFFER_SIZE);
    mSidechainRingBufferR = std::make_unique<float[]>(GlitchConfig::RING_BUFFER_SIZE);
    std::fill(mMainRingBufferL.get(), mMainRingBufferL.get() + GlitchConfig::RING_BUFFER_SIZE, 0.0f);
    std::fill(mMainRingBufferR.get(), mMainRingBufferR.get() + GlitchConfig::RING_BUFFER_SIZE, 0.0f);
    std::fill(mSidechainRingBufferL.get(), mSidechainRingBufferL.get() + GlitchConfig::RING_BUFFER_SIZE, 0.0f);
    std::fill(mSidechainRingBufferR.get(), mSidechainRingBufferR.get() + GlitchConfig::RING_BUFFER_SIZE, 0.0f);
    mWritePos = 0; mFilterL.reset(); mFilterR.reset();
    for (auto& lfo : mLFOs) lfo.reset();

    mCrossModL.prepare(sampleRate);
    mCrossModR.prepare(sampleRate);
    mScHpfL.reset();
    mScHpfR.reset();

    mSpectralL.prepare(samplesPerBlock); mSpectralR.prepare(samplesPerBlock);
    mMasterFX.prepare(sampleRate);
    mIsPrepared = true;
}

void GlitchBeatsAudioProcessor::releaseResources() {}

//==============================================================================
void GlitchBeatsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    float sensVal = paramSensitivity->load();
    float threshold = 0.5f * (1.0f - sensVal) + 0.001f;
    float grainMs = paramGrainSize->load();
    float chaosVal = paramChaos->load();
    float mixVal = paramMix->load();
    int routingMode = std::round(paramRouting->load());
    float smoothVal = paramSmooth->load();
    bool freezeParam = (paramFreeze->load() > 0.5f);
    bool effectiveFreeze = freezeParam;

    float pitchVal = paramPitch->load();
    float densityVal = paramDensity->load();
    int dirMode = std::round(paramDirection->load());
    int winShape = std::round(paramWinShape->load());
    float spreadVal = paramStereoSpread->load();

    int filterType = std::round(paramFilterType->load());
    float cutoffVal = paramCutoff->load();
    float resVal = paramResonance->load();
    bool filterBypass = (paramFilterBypass->load() > 0.5f);

    float masterGainDb = paramMasterGain->load();
    float masterMix = paramMasterMix->load();
    float masterCrush = paramMasterCrush->load();
    float masterDrive = paramMasterDrive->load(); // [New]
    float masterTone = paramMasterTone->load();   // [New]
    float masterResoTime = paramMasterResoTime->load();
    float masterResoFdbk = paramMasterResoFdbk->load();
    bool stutterOn = (paramMasterStutterOn->load() > 0.5f);
    int stutterDiv = std::round(paramMasterStutterDiv->load());

    int seqLength = std::round(paramSeqLength->load());
    int seqRate = std::round(paramSeqRate->load());
    bool seqBypass = (paramSeqBypass->load() > 0.5f);

    int xModMode = std::round(paramXModMode->load());
    float xModMix = paramXModMix->load();
    float xModParamA = paramXModParamA->load();
    float xModParamB = paramXModParamB->load();

    float scLowCut = paramSCLowCut->load();

    bool trigB = (paramTrigB->load() > 0.5f);

    double bpm = 120.0; double ppq = 0.0;
    if (auto* ph = getPlayHead()) { if (auto pos = ph->getPosition()) { bpm = pos->getBpm().orFallback(120.0); ppq = pos->getPpqPosition().orFallback(0.0); } }

    const int numSamples = buffer.getNumSamples();
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    auto mainBus = getBusBuffer(buffer, true, 0); auto sidechainBus = getBusBuffer(buffer, true, 1);
    const float* mainReadL = (mainBus.getNumChannels() > 0) ? mainBus.getReadPointer(0) : nullptr;
    const float* mainReadR = (mainBus.getNumChannels() > 1) ? mainBus.getReadPointer(1) : nullptr;
    const float* scReadL = (sidechainBus.getNumChannels() > 0) ? sidechainBus.getReadPointer(0) : nullptr;
    const float* scReadR = (sidechainBus.getNumChannels() > 1) ? sidechainBus.getReadPointer(1) : nullptr;

    float* outL = buffer.getWritePointer(0); float* outR = buffer.getWritePointer(1);
    int currentWritePos = mWritePos;
    const int mask = GlitchConfig::RING_BUFFER_MASK;
    const int bufSize = GlitchConfig::RING_BUFFER_SIZE;
    float currentSampleRate = (float)mSampleRate;
    float* destMainL = mMainRingBufferL.get(); float* destMainR = mMainRingBufferR.get();
    float* destScL = mSidechainRingBufferL.get(); float* destScR = mSidechainRingBufferR.get();
    float sumSquaredAnalysis = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        double currentPPQ = ppq + (double)i * (bpm / 60.0) / mSampleRate;

        // Multi-LFO
        float modSens = 0, modGrain = 0, modChaos = 0, modSmooth = 0, modPitch = 0, modCut = 0, modRes = 0, modMix = 0, modGain = 0, modXMix = 0, modXA = 0, modXB = 0, modCrush = 0, modSeq = 0, modStut = 0, modFilt = 0, modResTime = 0, modResFb = 0, modDrive = 0, modTone = 0;

        for (int l = 0; l < 6; ++l) {
            bool active = (paramLFOActive[l]->load() > 0.5f);
            if (!active) continue;

            int wave = std::round(paramLFOWave[l]->load());
            float freq = paramLFOFreq[l]->load();
            bool sync = (paramLFOSync[l]->load() > 0.5f);
            float lMin = paramLFOMin[l]->load() / 100.0f;
            float lMax = paramLFOMax[l]->load() / 100.0f;
            int target = std::round(paramLFOTarget[l]->load());
            float out = mLFOs[l].process(wave, freq, sync, true, mSampleRate, bpm, currentPPQ, mRandom);
            float val = lMin + (lMax - lMin) * out;
            switch (target) {
            case 0: modSens = val; break; case 1: modGrain = val; break; case 2: modChaos = val; break;
            case 3: modSmooth = val; break; case 4: modPitch = val; break; case 5: modCut = val; break;
            case 6: modRes = val; break; case 7: modMix = val; break; case 8: modGain = val; break;
            case 9: modXMix = val; break; case 10: modXA = val; break; case 11: modXB = val; break;
            case 12: modCrush = val; break; case 13: modSeq = val; break; case 14: modStut = val; break;
            case 15: modFilt = val; break;
            case 16: modResTime = val; break;
            case 17: modResFb = val; break;
            case 18: modDrive = val; break; // [New]
            case 19: modTone = val; break;  // [New]
            }
        }

        float effSens = sensVal; if (modSens > 0.001f) effSens = modSens;
        float effGrain = grainMs; if (modGrain > 0.001f) effGrain = 10.0f + modGrain * 490.0f;
        float effChaos = chaosVal; if (modChaos > 0.001f) effChaos = modChaos;
        float effSmooth = smoothVal; if (modSmooth > 0.001f) effSmooth = modSmooth;
        float effPitch = pitchVal; if (modPitch > 0.001f) effPitch = -12.0f + modPitch * 24.0f;
        float effCut = cutoffVal; if (modCut > 0.001f) effCut = 20.0f + modCut * 19980.0f;
        float effRes = resVal; if (modRes > 0.001f) effRes = modRes * 0.95f;
        float effMix = mixVal; if (modMix > 0.001f) effMix = modMix;
        float effGain = masterGainDb; if (modGain > 0.001f) effGain = -60.0f + modGain * 66.0f;
        float effXMix = xModMix; if (modXMix > 0.001f) effXMix = modXMix;
        float effXA = xModParamA; if (modXA > 0.001f) effXA = modXA;
        float effXB = xModParamB; if (modXB > 0.001f) effXB = modXB;
        float effCrush = masterCrush; if (modCrush > 0.001f) effCrush = modCrush;
        int effSeqRate = seqRate; if (modSeq > 0.001f) effSeqRate = std::clamp((int)(modSeq * 6.99f), 0, 6);
        int effStutDiv = stutterDiv; if (modStut > 0.001f) effStutDiv = std::clamp((int)(modStut * 4.99f), 0, 4);
        bool effFiltBy = filterBypass; if (modFilt > 0.001f) effFiltBy = (modFilt > 0.5f);

        float effResoTime = masterResoTime; if (modResTime > 0.001f) effResoTime = modResTime;
        float effResoFb = masterResoFdbk; if (modResFb > 0.001f) effResoFb = modResFb;

        float effDrive = masterDrive; if (modDrive > 0.001f) effDrive = modDrive;
        float effTone = masterTone; if (modTone > 0.001f) effTone = modTone;

        float threshold = 0.5f * (1.0f - effSens) + 0.001f;

        int currentStep = mSequencer.getCurrentStep(currentPPQ, seqLength, effSeqRate);
        mVisualSeqStep.store(currentStep);

        if (currentStep != mLastStep) {
            mLastStep = currentStep;
            mRndSens = mRandom.nextFloat();
            mRndGrain = 10.0f + mRandom.nextFloat() * 490.0f;
            mRndChaos = mRandom.nextFloat();
            mRndPitch = (float)(mRandom.nextInt(25) - 12);
        }

        int stepState = std::round(paramSeqSteps[static_cast<size_t>(currentStep)]->load());
        if (seqBypass) stepState = 1;

        if (stepState == 3) {
            threshold = 0.5f * (1.0f - mRndSens) + 0.001f;
            effGrain = mRndGrain;
            effChaos = mRndChaos;
            effPitch = mRndPitch;
        }

        bool allowSpawn = (stepState != 0);
        bool effectiveFreeze = (stepState == 2) || freezeParam || (trigB && currentStep == 0);
        bool forceStutter = (stepState == 4);

        int grainSamples = static_cast<int>(effGrain * (mSampleRate / 1000.0));
        float masterLin = juce::Decibels::decibelsToGain(effGain);

        float inL = (mainReadL) ? mainReadL[i] : 0.f; float inR = (mainReadR) ? mainReadR[i] : 0.f;
        float sideL = (scReadL) ? scReadL[i] : 0.f; float sideR = (scReadR) ? scReadR[i] : 0.f;

        if (!effectiveFreeze) { destMainL[currentWritePos] = inL; destMainR[currentWritePos] = inR; destScL[currentWritePos] = sideL; destScR[currentWritePos] = sideR; }

        float analysisSample = 0.0f; const float* sourceBufL = nullptr; const float* sourceBufR = nullptr;
        float dryL = inL, dryR = inR;
        switch (routingMode) {
        case 1: analysisSample = inL; sourceBufL = destMainL; sourceBufR = destMainR; break; // Self
        case 2: analysisSample = inL; sourceBufL = destScL; sourceBufR = destScR; dryL = sideL; dryR = sideR; break; // Swap
        default: analysisSample = sideL; sourceBufL = destMainL; sourceBufR = destMainR; break; // Ext
        }

        if (routingMode != 1) {
            analysisSample = mScHpfL.process(analysisSample, scLowCut, currentSampleRate);
        }

        sumSquaredAnalysis += (analysisSample * analysisSample);

        if (mDetector.process(analysisSample, threshold)) {
            if (allowSpawn) {
                mGlitchCore.spawn((float)currentWritePos, mRandom, bufSize, grainSamples, effChaos, effPitch,
                    densityVal, dirMode, winShape, spreadVal);
                mVisualTriggerFired.store(true);
            }
        }

        float glitchL = 0.0f, glitchR = 0.0f;
        mGlitchCore.process(sourceBufL, sourceBufR, mask, glitchL, glitchR, effSmooth);

        float mixedL = dryL * (1.0f - effMix) + glitchL * effMix;
        float mixedR = dryR * (1.0f - effMix) + glitchR * effMix;

        mixedL = mFilterL.processStudio(mixedL, filterType, effCut, effRes, currentSampleRate, effFiltBy);
        mixedR = mFilterR.processStudio(mixedR, filterType, effCut, effRes, currentSampleRate, effFiltBy);

        float modL = 0.0f; float modR = 0.0f;
        if (xModMode == 0) { modL = mSpectralL.processSample(mixedL, sideL, effXA, effXB); modR = mSpectralR.processSample(mixedR, sideR, effXA, effXB); }
        else if (xModMode == 1) { modL = mCrossModL.processAM(mixedL, sideL, effXA, effXB); modR = mCrossModR.processAM(mixedR, sideR, effXA, effXB); }
        else { modL = mCrossModL.processFM(mixedL, sideL, effXA, effXB); modR = mCrossModR.processFM(mixedR, sideR, effXA, effXB); }

        mixedL = mixedL * (1.0f - effXMix) + modL * effXMix;
        mixedR = mixedR * (1.0f - effXMix) + modR * effXMix;

        bool activeStutter = stutterOn || forceStutter;

        mMasterFX.process(mixedL, mixedR, effCrush, effDrive, effTone, effResoTime, effResoFb,
            activeStutter, effStutDiv, bpm, masterMix, effGain);

        // Visualizer Update
        int visIdx = mVisWritePos.load(std::memory_order_relaxed);
        mVisDryL[visIdx] = dryL;
        mVisWetL[visIdx] = mixedL;
        mVisWritePos.store((visIdx + 1) % VIS_SIZE, std::memory_order_relaxed);

        outL[i] = mixedL; outR[i] = mixedR;

        if (!effectiveFreeze) currentWritePos = (currentWritePos + 1) & mask;
    }
    mWritePos = currentWritePos;

    if (numSamples > 0) mVisualSidechainRMS.store(std::sqrt(sumSquaredAnalysis / numSamples));
}

bool GlitchBeatsAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* GlitchBeatsAudioProcessor::createEditor() { return new PluginEditor(*this); }
const juce::String GlitchBeatsAudioProcessor::getName() const { return "GlitchNexus"; }
bool GlitchBeatsAudioProcessor::acceptsMidi() const { return false; }
bool GlitchBeatsAudioProcessor::producesMidi() const { return false; }
bool GlitchBeatsAudioProcessor::isMidiEffect() const { return false; }
double GlitchBeatsAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int GlitchBeatsAudioProcessor::getNumPrograms() { return 1; }
int GlitchBeatsAudioProcessor::getCurrentProgram() { return 0; }
void GlitchBeatsAudioProcessor::setCurrentProgram(int index) {}
const juce::String GlitchBeatsAudioProcessor::getProgramName(int index) { return {}; }
void GlitchBeatsAudioProcessor::changeProgramName(int index, const juce::String& newName) {}
void GlitchBeatsAudioProcessor::getStateInformation(juce::MemoryBlock& destData) { auto state = parameters.copyState(); std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml, destData); }
void GlitchBeatsAudioProcessor::setStateInformation(const void* data, int sizeInBytes) { std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes)); if (xmlState.get() != nullptr) if (xmlState->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xmlState)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GlitchBeatsAudioProcessor(); }