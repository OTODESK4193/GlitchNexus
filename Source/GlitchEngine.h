/*
  ==============================================================================

    GlitchEngine.h (Phase 44)
    Created: 2026
    Author: OTODESK (Glitch Arch)
    Description: DSP Engine - Added Drive & Tone to MasterFX

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <memory>
#include <algorithm>
#include <complex>

namespace GlitchConfig
{
    constexpr int RING_BUFFER_SIZE = 131072;
    constexpr int RING_BUFFER_MASK = RING_BUFFER_SIZE - 1;
    constexpr int MAX_GRAINS = 64;
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr int FFT_ORDER = 10;
    constexpr int FFT_SIZE = 1 << FFT_ORDER;
}

//==============================================================================
// Simple HPF
//==============================================================================
class SimpleHPF
{
public:
    void reset() { lastX = 0.0f; lastY = 0.0f; }
    float process(float input, float cutoff, float sampleRate)
    {
        if (cutoff <= 5.0f) return input;

        float rc = 1.0f / (GlitchConfig::TWO_PI * cutoff + 0.001f);
        float dt = 1.0f / sampleRate;
        float a = rc / (rc + dt);

        float y = a * (lastY + input - lastX);

        if (std::isnan(y) || std::isinf(y)) y = 0.0f;

        lastX = input;
        lastY = y;
        return y;
    }
private:
    float lastX = 0.0f;
    float lastY = 0.0f;
};

//==============================================================================
// Transient Detector
//==============================================================================
class TransientDetector
{
public:
    bool process(float sample, float threshold)
    {
        float absVal = std::abs(sample);
        mEnvelope = absVal > mEnvelope ? absVal : mEnvelope * 0.999f;
        if (absVal > threshold && !mTriggered) { mTriggered = true; mHoldCounter = 1000; return true; }
        if (mHoldCounter > 0) mHoldCounter--; else mTriggered = false;
        return false;
    }
private:
    float mEnvelope = 0.0f; bool mTriggered = false; int mHoldCounter = 0;
};

//==============================================================================
// Glitch Core
//==============================================================================
struct Grain
{
    bool isActive = false; float position = 0.0f; float speed = 1.0f;
    float lifeTime = 0.0f; float age = 0.0f; float volume = 1.0f; float pan = 0.5f;
    int windowType = 0;
};

class GlitchCore
{
public:
    GlitchCore() { for (auto& g : mGrains) g.isActive = false; }
    void spawn(float writePos, juce::Random& r, int bufSize, int sizeSamples, float chaosVal, float pitchSemi,
        float density, int directionMode, int winShape, float spread)
    {
        int spawnCount = 1;
        if (density > 0.3f && r.nextFloat() < density) spawnCount++;
        if (density > 0.7f && r.nextFloat() < density) spawnCount++;

        for (int k = 0; k < spawnCount; ++k) {
            for (auto& g : mGrains) {
                if (!g.isActive) {
                    g.isActive = true; g.age = 0.0f; g.windowType = winShape;
                    float sizeVar = 1.0f + (r.nextFloat() * 2.0f - 1.0f) * chaosVal;
                    g.lifeTime = (float)sizeSamples * sizeVar; if (g.lifeTime < 100.0f) g.lifeTime = 100.0f;
                    float offset = r.nextFloat() * chaosVal * (float)bufSize * 0.1f;
                    g.position = writePos - offset; if (g.position < 0) g.position += bufSize;

                    float pitchMult = std::pow(2.0f, pitchSemi / 12.0f);
                    float chaosPitch = 1.0f + (r.nextFloat() * 2.0f - 1.0f) * chaosVal * 0.1f;
                    float baseSpeed = pitchMult * chaosPitch;

                    bool reverse = false;
                    if (directionMode == 1) reverse = true;
                    else if (directionMode == 2) reverse = (mLastDirWasRev = !mLastDirWasRev);
                    else if (directionMode == 3) reverse = (r.nextFloat() > 0.5f);
                    g.speed = reverse ? -baseSpeed : baseSpeed;

                    float rndPan = (r.nextFloat() * 2.0f - 1.0f);
                    g.pan = 0.5f + (rndPan * 0.5f * spread);
                    g.pan = std::clamp(g.pan, 0.0f, 1.0f);
                    g.volume = 0.8f;
                    break;
                }
            }
        }
    }
    void process(const float* bufL, const float* bufR, int mask, float& outL, float& outR, float smoothVal)
    {
        float gL = 0.0f; float gR = 0.0f; int activeCount = 0;
        for (auto& g : mGrains) {
            if (!g.isActive) continue;

            int iPos = (int)std::floor(g.position);
            float frac = g.position - (float)iPos;

            int idx0 = iPos & mask;
            int idx1 = (iPos + 1) & mask;

            float sampL = bufL[idx0] + frac * (bufL[idx1] - bufL[idx0]);
            float sampR = bufR[idx0] + frac * (bufR[idx1] - bufR[idx0]);

            float n = g.age / g.lifeTime; float env = 0.0f;
            switch (g.windowType) {
            case 0: {
                float fadeLen = g.lifeTime * smoothVal * 0.5f; if (fadeLen < 1.f) fadeLen = 1.f;
                if (g.age < fadeLen) env = g.age / fadeLen; else if (g.age > g.lifeTime - fadeLen) env = (g.lifeTime - g.age) / fadeLen; else env = 1.0f; break;
            }
            case 1: env = 0.5f * (1.0f - std::cos(GlitchConfig::TWO_PI * n)); break;
            case 2: if (n < 0.01f) env = n * 100.f; else if (n > 0.99f) env = (1.0f - n) * 100.f; else env = 1.0f; break;
            case 3: env = 1.0f - n; break;
            }
            gL += sampL * (1.0f - g.pan) * g.volume * env; gR += sampR * g.pan * g.volume * env;
            g.position += g.speed; g.age += 1.0f; if (g.age >= g.lifeTime) g.isActive = false;
            activeCount++;
        }
        if (activeCount > 0) { float norm = 1.0f / std::sqrt((float)activeCount + 1.0f); outL = gL * norm; outR = gR * norm; }
    }

    const std::array<Grain, GlitchConfig::MAX_GRAINS>& getGrains() const { return mGrains; }

private:
    std::array<Grain, GlitchConfig::MAX_GRAINS> mGrains;
    bool mLastDirWasRev = false;
};

//==============================================================================
// Glitch Filter
//==============================================================================
class GlitchFilter
{
public:
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; sCutoff = 20000.0f; sRes = 0.0f; }
    float processStudio(float input, int type, float targetCutoff, float targetRes, float sampleRate, bool bypass)
    {
        if (bypass) return input;
        sCutoff = sCutoff * 0.95f + targetCutoff * 0.05f; sRes = sRes * 0.95f + targetRes * 0.05f;
        float w0 = GlitchConfig::TWO_PI * sCutoff / sampleRate;
        float cosW = std::cos(w0); float sinW = std::sin(w0);
        float qVal = 0.707f + (sRes * 9.0f); float alpha = sinW / (2.0f * qVal);
        float b0 = 0, b1 = 0, b2 = 0, a0 = 0, a1 = 0, a2 = 0;
        switch (type) {
        case 0: b0 = (1.f - cosW) / 2.f; b1 = 1.f - cosW; b2 = (1.f - cosW) / 2.f; a0 = 1.f + alpha; a1 = -2.f * cosW; a2 = 1.f - alpha; break;
        case 1: b0 = (1.f + cosW) / 2.f; b1 = -(1.f + cosW); b2 = (1.f + cosW) / 2.f; a0 = 1.f + alpha; a1 = -2.f * cosW; a2 = 1.f - alpha; break;
        case 2: b0 = alpha; b1 = 0.f; b2 = -alpha; a0 = 1.f + alpha; a1 = -2.f * cosW; a2 = 1.f - alpha; break;
        case 3: b0 = 1.f; b1 = -2.f * cosW; b2 = 1.f; a0 = 1.f + alpha; a1 = -2.f * cosW; a2 = 1.f - alpha; break;
        default: return input;
        }
        float invA0 = 1.0f / a0; b0 *= invA0; b1 *= invA0; b2 *= invA0; a1 *= invA0; a2 *= invA0;
        float output = b0 * input + b1 * z1[0] + b2 * z2[0] - a1 * z1[1] - a2 * z2[1];
        if (std::abs(output) < 1.0e-15f) output = 0.0f;
        z2[0] = z1[0]; z1[0] = input; z2[1] = z1[1]; z1[1] = output;
        return output;
    }
private:
    float z1[2] = { 0.0f }; float z2[2] = { 0.0f }; float sCutoff = 20000.0f; float sRes = 0.0f;
};

//==============================================================================
// Glitch LFO
//==============================================================================
class GlitchLFO
{
public:
    void reset() { phase = 0.0f; lastOut = 0.0f; }
    float process(int wave, float freq, bool sync, bool active, double sampleRate, double bpm, double ppq, juce::Random& r)
    {
        if (!active) return 0.0f;

        float currentPhase = 0.0f;
        if (sync) {
            float beats = 4.0f;
            if (freq > 0.8f) beats = 0.25f; else if (freq > 0.6f) beats = 0.5f; else if (freq > 0.4f) beats = 1.0f; else if (freq > 0.2f) beats = 2.0f; else beats = 4.0f;
            double beatPos = ppq / beats; currentPhase = (float)(beatPos - std::floor(beatPos));
        }
        else {
            float hz = 0.01f * std::pow(3000.0f, freq); phase += hz / (float)sampleRate;
            if (phase >= 1.0f) phase -= 1.0f; currentPhase = phase;
        }
        switch (wave) {
        case 0: return std::sin(currentPhase * GlitchConfig::TWO_PI) * 0.5f + 0.5f; // Sine
        case 1: return (currentPhase < 0.5f) ? 2.0f * currentPhase : 2.0f - 2.0f * currentPhase; // Tri
        case 2: return currentPhase; // Saw
        case 3: return (currentPhase < 0.5f) ? 1.0f : 0.0f; // Square
        case 4: if (currentPhase < lastPhase) lastOut = r.nextFloat(); lastPhase = currentPhase; return lastOut; // S&H
        case 5: return r.nextFloat(); // Noise
        }
        return 0.0f;
    }
private:
    float phase = 0.0f; float lastPhase = 0.0f; float lastOut = 0.0f;
};

//==============================================================================
// X-Mod Components
//==============================================================================
class SpectralMorph
{
public:
    void prepare(int) {}
    float processSample(float carrier, float modulator, float paramA, float paramB)
    {
        float drive = 1.0f + paramA * 10.0f;
        float mixInput = (carrier + modulator * 0.5f) * drive;
        float fold = std::sin(mixInput);
        if (paramB > 0.0f) fold = std::sin(fold * (1.0f + paramB * 4.0f));
        return fold;
    }
};

class CrossModulator
{
public:
    void prepare(double sampleRate) {
        int size = (int)(sampleRate * 0.1) + 1;
        mBuffer = std::make_unique<float[]>(size);
        mBufferSize = size; mWritePos = 0; mSampleRate = sampleRate;
        std::fill(mBuffer.get(), mBuffer.get() + size, 0.0f);
        mFilterZ = 0.0f;
    }
    float processAM(float carrier, float modulator, float paramA, float paramB)
    {
        float drive = 1.0f + paramA * 20.0f;
        float drivenCar = std::tanh(carrier * drive);
        float drivenMod = std::tanh(modulator * drive);
        float ring = drivenCar * drivenMod;
        if (ring < 0.0f) ring *= 0.8f;
        float cutoff = 1.0f - paramB;
        mFilterZ = mFilterZ * (1.0f - cutoff) + ring * cutoff;
        return mFilterZ;
    }
    float processFM(float carrier, float modulator, float paramA, float paramB)
    {
        if (mBufferSize == 0) return carrier;
        float input = carrier + mLastOutput * paramB * 0.9f;
        mBuffer[mWritePos] = input;
        float baseDelay = 100.0f;
        float modDelay = std::abs(modulator) * paramA * 500.0f;
        float readPos = (float)mWritePos - (baseDelay + modDelay);
        while (readPos < 0) readPos += mBufferSize;
        while (readPos >= mBufferSize) readPos -= mBufferSize;

        int iPos = (int)readPos;
        float frac = readPos - iPos;
        int iNext = (iPos + 1) % mBufferSize;

        float val = mBuffer[iPos] + frac * (mBuffer[iNext] - mBuffer[iPos]);
        mLastOutput = val;
        mWritePos = (mWritePos + 1) % mBufferSize;
        return val;
    }
private:
    std::unique_ptr<float[]> mBuffer; int mBufferSize = 0; int mWritePos = 0;
    float mLastOutput = 0.0f; float mFilterZ = 0.0f; double mSampleRate = 44100.0;
};

class GlitchSequencer
{
public:
    int getCurrentStep(double ppq, int length, int rateIdx) {
        double subdivision = 1.0;
        switch (rateIdx) { case 0: subdivision = 4.0; break; case 1: subdivision = 2.0; break; case 2: subdivision = 1.0; break; case 3: subdivision = 0.5; break; case 4: subdivision = 0.25; break; case 5: subdivision = 0.125; break; case 6: subdivision = 0.0625; break; }
                                 double totalSteps = ppq / subdivision; int step = (int)std::floor(totalSteps); if (length < 1) length = 1;
                                 return step % length;
    }
};

//==============================================================================
// Master FX
//==============================================================================
class MasterFX
{
public:
    void prepare(double sampleRate) {
        mSampleRate = sampleRate;
        mBuffer = std::make_unique<float[]>(192000);

        int resBufSize = (int)(sampleRate * 0.1) + 1;
        mResoBufferL = std::make_unique<float[]>(resBufSize);
        mResoBufferR = std::make_unique<float[]>(resBufSize);
        mResoSize = resBufSize;
        std::fill(mResoBufferL.get(), mResoBufferL.get() + resBufSize, 0.0f);
        std::fill(mResoBufferR.get(), mResoBufferR.get() + resBufSize, 0.0f);

        mTiltL = 0.0f; mTiltR = 0.0f;
    }

    void process(float& l, float& r, float crush, float drive, float tone,
        float resoTime, float resoFdbk,
        bool stutter, int stutterDiv, double bpm, float mix, float gainDb)
    {
        float dryL = l;
        float dryR = r;

        // --- 1. BitCrush & Downsample ---
        if (crush > 0.0f) {
            float quant = 1.0f - crush * 0.8f;
            float rateRed = 1.0f + crush * 49.0f;

            mDownsampleCount += 1.0f;
            if (mDownsampleCount >= rateRed) {
                mDownsampleCount -= rateRed;
                mHoldSampleL = std::floor(l / quant) * quant;
                mHoldSampleR = std::floor(r / quant) * quant;
            }
            l = mHoldSampleL;
            r = mHoldSampleR;
        }

        // --- 2. Drive (Saturation) [New] ---
        if (drive > 0.0f) {
            float driveAmt = 1.0f + drive * 4.0f;
            l = std::tanh(l * driveAmt);
            r = std::tanh(r * driveAmt);
        }

        // --- 3. Resonator ---
        if (resoTime > 0.0f) {
            float delayMs = 1.0f + resoTime * 29.0f;
            float delaySamples = delayMs * (float)mSampleRate / 1000.0f;
            float feedback = resoFdbk * 0.95f;

            float readPos = (float)mResoPos - delaySamples;
            while (readPos < 0) readPos += mResoSize;
            int iPos = (int)readPos;
            float frac = readPos - iPos;
            int iNext = (iPos + 1) % mResoSize;

            float dL = mResoBufferL[iPos] + frac * (mResoBufferL[iNext] - mResoBufferL[iPos]);
            float dR = mResoBufferR[iPos] + frac * (mResoBufferR[iNext] - mResoBufferR[iPos]);

            float inL = l + dL * feedback;
            float inR = r + dR * feedback;

            mResoBufferL[mResoPos] = inL;
            mResoBufferR[mResoPos] = inR;

            mResoPos = (mResoPos + 1) % mResoSize;

            l = inL;
            r = inR;
        }

        // --- 4. Stutter ---
        mBuffer[mWrite] = l; mBuffer[mWrite + 1] = r;
        mWrite = (mWrite + 2) % 192000;

        if (stutter) {
            float beats = 1.0f;
        switch (stutterDiv) { case 0: beats = 1.0f; break; case 1: beats = 0.5f; break; case 2: beats = 0.25f; break; case 3: beats = 0.125f; break; case 4: beats = 0.0625f; break; }
                                    int loopLen = (int)((mSampleRate * 60.0 / bpm) * beats) * 2; if (loopLen < 2) loopLen = 2;
                                    if (!mWasStutter) { mRead = (mWrite - loopLen + 192000) % 192000; mWasStutter = true; }
                                    int readIdx = (mRead + mCount) % 192000; l = mBuffer[readIdx]; r = mBuffer[readIdx + 1];
                                    mCount = (mCount + 2) % loopLen;
        }
        else { mWasStutter = false; mCount = 0; }

        // --- 5. Tilt EQ [New] ---
        // Simple 1-pole Tilt approximation (Center 0.5)
        // tone 0.5 = neutral. 0.0 = dark. 1.0 = bright.
        float tiltAlpha = 0.3f; // ~1kHz at 44.1k
        mTiltL += tiltAlpha * (l - mTiltL);
        mTiltR += tiltAlpha * (r - mTiltR);
        float lowL = mTiltL; float highL = l - mTiltL;
        float lowR = mTiltR; float highR = r - mTiltR;

        float lowGain = (1.0f - tone) * 2.0f;
        float highGain = tone * 2.0f;

        l = lowL * lowGain + highL * highGain;
        r = lowR * lowGain + highR * highGain;

        // --- 6. Mix & Gain ---
        l = dryL * (1.0f - mix) + l * mix;
        r = dryR * (1.0f - mix) + r * mix;

        float gainLin = juce::Decibels::decibelsToGain(gainDb);
        l *= gainLin;
        r *= gainLin;
    }

private:
    std::unique_ptr<float[]> mBuffer;
    std::unique_ptr<float[]> mResoBufferL;
    std::unique_ptr<float[]> mResoBufferR;
    int mWrite = 0; int mRead = 0; int mCount = 0; bool mWasStutter = false;
    double mSampleRate = 44100.0;

    int mResoPos = 0;
    int mResoSize = 0;
    float mDownsampleCount = 0.0f;
    float mHoldSampleL = 0.0f;
    float mHoldSampleR = 0.0f;
    float mTiltL = 0.0f;
    float mTiltR = 0.0f;
};