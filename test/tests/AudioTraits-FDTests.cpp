//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"
#include "SignalGenerator.hpp"

#include <algorithm>
#include <vector>

#include "AudioTraits.hpp"
#include "Utils.hpp"
#include "AudioFileSignalAdapter.hpp"

//#include "matplotlibcpp.h"

using namespace slb;
using namespace AudioTraits;
using namespace TestCommon;


TEST_CASE("AudioTraits::FrequencyDomain: basic tests")
{
    constexpr float sampleRate = 48e3f;
    int signalLength = static_cast<int>(sampleRate) * 1;
    
    SECTION("Invalid Parameters") {
        std::vector<std::vector<float>> empty;
        SignalAdapterStdVecVec emptySignal(empty);

        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(emptySignal, {}, FrequencySelection{}, sampleRate));
        
        std::vector<std::vector<float>> minimal(1, std::vector<float>(1));
        SignalAdapterStdVecVec minSig(minimal);
        
        REQUIRE_THROWS(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{0}, sampleRate)); // DC is not valid
        REQUIRE_THROWS(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{0, 1000}, sampleRate)); // DC is not valid
        REQUIRE_NOTHROW(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{.001f, 1000}, sampleRate)); // .001Hz is valid
        REQUIRE_THROWS(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{-10}, sampleRate)); // negative frequency is not valid
        REQUIRE_THROWS(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{{-10, 200}}, sampleRate)); // range that starts in negative is also invalid
        REQUIRE_THROWS(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2+10}, sampleRate)); // above Nyquist is invalid
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{1}, sampleRate)); // 1Hz is ok
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2}, sampleRate)); // Nyquist is ok
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{}, sampleRate)); // no frequency is always false
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(minSig, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(minSig, {}, FrequencySelection{1000}, sampleRate));
        
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{0}, sampleRate)); // DC is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{-10}, sampleRate)); // negative frequency is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{{-10, 200}}, sampleRate)); // range that starts in negative is also invalid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2+10}, sampleRate)); // above Nyquist is invalid
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{1}, sampleRate)); // true because there's no signal
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2}, sampleRate)); // true because there's no signal
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{}, sampleRate)); // true because there's no signal
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(minSig, {}, FrequencySelection{1000}, sampleRate));
    }
    
    SECTION("Silence") {
        auto silence = SignalGenerator::createSilence<float>(signalLength);
        std::vector<std::vector<float>> silenceData { silence, silence };
        SignalAdapterStdVecVec silenceSignal(silenceData);
        
        // check always has to return false
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(silenceSignal, {}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(silenceSignal, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(silenceSignal, {2}, FrequencySelection{sampleRate/2}, sampleRate));
        
        // check always has to return true - silence is always legal
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {}, FrequencySelection{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{sampleRate/2}, sampleRate));
    }
}

TEST_CASE("AudioTraits::FrequencyDomain: sine tests")
{
    constexpr float sampleRate = 48e3f;
    int signalLength = static_cast<int>(sampleRate) * 1;
    
    float gain_dB = GENERATE(0.f, +3.f, -3.f, -50.f);
    auto sine1kSignal = SignalGenerator::createSine<float>(1000, sampleRate, signalLength, gain_dB);
    auto sine2kSignal = SignalGenerator::createSine<float>(2000, sampleRate, signalLength, gain_dB);
    auto sine6kSignal = SignalGenerator::createSine<float>(6000, sampleRate, signalLength, gain_dB);
    std::vector<std::vector<float>> sineData {sine1kSignal, sine2kSignal, sine6kSignal};
    SignalAdapterStdVecVec sine(sineData);
    
    REQUIRE(sampleRate == 48000);
    
    SECTION("Single Frequency Ranges") {
        // 1kHz sine
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(sine, {1}, FrequencySelection{}, sampleRate)); // no frequency is always false
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{}, sampleRate)); // no frequency is always false
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sine, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sine, {1}, FrequencySelection{1007}, sampleRate)); // +7Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1007}, sampleRate)); // +7Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1008}, sampleRate)); // +8Hz is outside bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{{900, 1100}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{2000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{900}, sampleRate));
        
        // 2kHz sine
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sine, {2}, FrequencySelection{2000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2000}, sampleRate));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sine, {2}, FrequencySelection{2014}, sampleRate)); // +14Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2014}, sampleRate)); // +14Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2016}, sampleRate)); // +16Hz is outside bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{{1950, 2050}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2050}, sampleRate)); // 50 Hz is enough to be in next bin at this fs
        
        // multichannel: 1k in L and 2k in R
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sine, {1, 2}, FrequencySelection{{1000, 2000}}, sampleRate, -2)); // both channels are within this range
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 2}, FrequencySelection{{1000, 2000}}, sampleRate, -2)); // both channels are within this range
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 2}, FrequencySelection{{900, 1500}}, sampleRate)); // R is outside this range
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 2}, FrequencySelection{{20, 900}}, sampleRate)); // No signal
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 2}, FrequencySelection{{2100, sampleRate/2}}, sampleRate)); // No signal
        
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{1000, 6000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{2000, 6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2, 3}, FrequencySelection{{2000, 6000}}, sampleRate));
        
        // set specific threshold
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1000}, sampleRate, -5.f));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2000}, sampleRate, -5.f));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1000}, sampleRate, -12.f));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sine, {1}, FrequencySelection{500}, sampleRate, -120)); // super low threshold always true
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(sine, {1}, FrequencySelection{1000}, sampleRate, +.0001f)); // positive threshold cannot be true.
    }
    
    SECTION("Multiple Frequency Ranges") {
        // Sine has 1000, 2000 and 6000 components in channels 1,2,3 respectively
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{1000}, {2000}, {6000}}, sampleRate)); // All channels have signals in valid ranges
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{1000}, {6000}}, sampleRate)); // 2000 Hz is not valid, check in channel 2 fails
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(sine, {}, FrequencySelection{{1000}, {6000}}, sampleRate)); // one range fails for ch 1 and 3, both fail for ch 2
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{500}, {1000}, {2000}, {6000}}, sampleRate)); // 500 Hz is not present in signal
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(sine, {}, FrequencySelection{{500}, {1000}, {2000}, {6000}}, sampleRate)); // 500 Hz is not present in signal
        
        // exclude channel 2
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 3}, FrequencySelection{{1000}, {6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 3}, FrequencySelection{{1000, 6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 3}, FrequencySelection{{600, 1200}, {2100, 10000}}, sampleRate));
        
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 2}, FrequencySelection{{600, 1200}, {2100, 10000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{600, 1200}, {2100, 10000}}, sampleRate));
    }
}

TEST_CASE("AudioTraits::FrequencyDomain: multi-sine tests")
{
    constexpr float sampleRate = 48e3f;
    int signalLength = static_cast<int>(sampleRate) * 1;
    
    SECTION("Summed Sine Waves in a Channel") {
        auto sine1kSignal = SignalGenerator::createSine<float>(1000, sampleRate, signalLength);
        auto sine2kSignal = SignalGenerator::createSine<float>(2000, sampleRate, signalLength);
        
        std::vector<float> sine1k2kSignal(signalLength);
        for (int i=0; i < signalLength; ++i) {
            sine1k2kSignal[i] = sine1kSignal[i] + sine2kSignal[i];
        }
        std::vector<std::vector<float>> sinesData {sine1k2kSignal};
        SignalAdapterStdVecVec sines(sinesData);
        
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{1000},{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{1000}}, sampleRate));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{900,1100},{1800,2200}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{1050,1100},{2000}}, sampleRate)); // 1k is no longer in range

        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1000},{2000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{2000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1050,1100},{2000}}, sampleRate));
    }
    SECTION("Summed Sines with different amplitudes") {
        float thrs = GENERATE(-1.f, -10.f, -40.f); //dB
        
        auto sine1kSignal = SignalGenerator::createSine<float>(1000, sampleRate, signalLength, 0.f);
        auto sine2kSignal = SignalGenerator::createSine<float>(2000, sampleRate, signalLength, thrs+0.1f); // just above the threshold
        
        std::vector<float> sine1k2kSignal(signalLength);
        for (int i=0; i < signalLength; ++i) {
            sine1k2kSignal[i] = sine1kSignal[i] + sine2kSignal[i];
        }
        std::vector<std::vector<float>> sinesData {sine1k2kSignal};
        SignalAdapterStdVecVec sines(sinesData);
        
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{1000},{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{1000},{2000}}, sampleRate, thrs));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{1000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllFrequencyRanges>(sines, {}, FrequencySelection{{2000}}, sampleRate, thrs));

        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1000},{2000}}, sampleRate));
        if (thrs > -20.f) { // for lower thresholds, noise gets too high
            REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1000},{2000}}, sampleRate, thrs));
        }
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1000}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{2000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{2000}}, sampleRate, thrs)); // still fale because of the 1k signal
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sines, {}, FrequencySelection{{1050,1100},{2000}}, sampleRate));
    }
}
    
TEST_CASE("AudioTraits::FrequencyDomain: noise tests")
{
    constexpr float sampleRate = 48e3f;
    SECTION("White Noise (unfiltered)") {
        // Extreme case (Ultra-low frequency, very long signal)
        {
            int signalLength = static_cast<int>(sampleRate) * 35;
            auto noiseSignal = SignalGenerator::createWhiteNoise(signalLength);
            std::vector<std::vector<float>> longNoiseData {noiseSignal, noiseSignal};
            SignalAdapterStdVecVec longNoise(longNoiseData);
            
            // One range emcompasses all bins -> cannot be false
            REQUIRE(check<HasSignalInAllFrequencyRanges>(longNoise, {}, FrequencySelection{{1, sampleRate/2}}, sampleRate));
            
            // second bin at 2*ceil(48000/4096)=24Hz
            REQUIRE(sampleRate == 48000);
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(longNoise, {}, FrequencySelection{{24, sampleRate/2}}, sampleRate));
        }
        // Normal cases
        {
            float gain_dB = GENERATE(0.f, +3.f, -3.f, -50.f);
            int signalLength = static_cast<int>(sampleRate) * 10; // need longer signal
            auto noiseSignal = SignalGenerator::createWhiteNoise(signalLength, gain_dB);
            std::vector<std::vector<float>> noiseData {noiseSignal};
            SignalAdapterStdVecVec noise(noiseData);
            
            // lower threshold because signal is not long enough
            float thrs = -3.f;
            
            // positive threshold, cannot be true.
            REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{{20, sampleRate/2}}, sampleRate, +.5f));
            
            // Other than the case above, white noise should always result in true checks
            REQUIRE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{1000}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{10000}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{100}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{sampleRate/2}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{{1000, sampleRate/2}}, sampleRate, thrs));
            
            // maximum possible frequency range
            REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{1, sampleRate/2}}, sampleRate));
            // Other than the case above, white noise should always result in false checks
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{1000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{10000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{100}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{sampleRate/2}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{1000, sampleRate/2}}, sampleRate));
        }
    }
    
    SECTION("Band-limited Noise (filtered)")
    {
        AudioFile<float> bandlimitedNoise(std::string(TOSTRING(SOURCE_DIR)) + "/test/test_data/BandLimitedNoise_1k_4k.wav");
        ASSERT(bandlimitedNoise.getSampleRate() == sampleRate);

        const float gain_dB = 0.f; // hard-coded into file
        constexpr float lowerFreq = 1000;
        constexpr float upperFreq = 4000;
        REQUIRE(upperFreq-lowerFreq > 100); // test build on this
        constexpr float centerPoint = lowerFreq + (upperFreq-lowerFreq)/2;
        
        // apply gain
        for (auto& s : FrequencyDomainHelpers::getNormalizedBinValues(bandlimitedNoise.samples[0])) {
            s *= Utils::dB2Linear(gain_dB);
        }
        
        SignalAdapterStdVecVec noise(bandlimitedNoise.samples);
        
        float thrs = -5.9f; // This seems to be the amount of precision we can currently get with band-limited noise
        
        REQUIRE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{{20, 400}, {lowerFreq, upperFreq}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalInAllFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq}, {10000, sampleRate/2}}, sampleRate, thrs));

        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalOnlyAbove>(noise, {}, centerPoint, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyAbove>(noise, {}, lowerFreq, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyBelow>(noise, {}, upperFreq, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*0.8f, upperFreq*1.5f}}, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*0.9f, upperFreq}}, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq*1.1f}}, sampleRate, thrs));
        
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*1.1f, upperFreq}}, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*1.001f, upperFreq}}, sampleRate, thrs));

        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq*0.9f}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*1.1f, upperFreq*0.9f}}, sampleRate, thrs));
    }
}
