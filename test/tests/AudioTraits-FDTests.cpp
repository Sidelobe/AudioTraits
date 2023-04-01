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

        REQUIRE_THROWS(check<HasSignalOnlyInBands>(emptySignal, {}, Freqs{}, sampleRate));
        
        std::vector<std::vector<float>> minimal(1, std::vector<float>(1));
        SignalAdapterStdVecVec minSig(minimal);
        
        REQUIRE_THROWS(check<HasSignalInAllBands>(minSig, {1}, Freqs{0}, sampleRate)); // DC is not valid
        REQUIRE_THROWS(check<HasSignalInAllBands>(minSig, {1}, Freqs{0, 1000}, sampleRate)); // DC is not valid
        REQUIRE_NOTHROW(check<HasSignalInAllBands>(minSig, {1}, Freqs{.001f, 1000}, sampleRate)); // .001Hz is valid
        REQUIRE_THROWS(check<HasSignalInAllBands>(minSig, {1}, Freqs{-10}, sampleRate)); // negative frequency is not valid
        REQUIRE_THROWS(check<HasSignalInAllBands>(minSig, {1}, Freqs{{-10, 200}}, sampleRate)); // range that starts in negative is also invalid
        REQUIRE_THROWS(check<HasSignalInAllBands>(minSig, {1}, Freqs{sampleRate/2+10}, sampleRate)); // above Nyquist is invalid
        REQUIRE_FALSE(check<HasSignalInAllBands>(minSig, {1}, Freqs{1}, sampleRate)); // 1Hz is ok
        REQUIRE_FALSE(check<HasSignalInAllBands>(minSig, {1}, Freqs{sampleRate/2}, sampleRate)); // Nyquist is ok
        REQUIRE_FALSE(check<HasSignalInAllBands>(minSig, {1}, Freqs{}, sampleRate)); // no frequency is always false
        REQUIRE_FALSE(check<HasSignalInAllBands>(minSig, {1}, Freqs{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllBands>(minSig, {}, Freqs{1000}, sampleRate));
        
        REQUIRE_THROWS(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{0}, sampleRate)); // DC is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{-10}, sampleRate)); // negative frequency is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{{-10, 200}}, sampleRate)); // range that starts in negative is also invalid
        REQUIRE_THROWS(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{sampleRate/2+10}, sampleRate)); // above Nyquist is invalid
        REQUIRE(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{1}, sampleRate)); // true because there's no signal
        REQUIRE(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{sampleRate/2}, sampleRate)); // true because there's no signal
        REQUIRE(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{}, sampleRate)); // true because there's no signal
        REQUIRE(check<HasSignalOnlyInBands>(minSig, {1}, Freqs{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(minSig, {}, Freqs{1000}, sampleRate));
    }
    
    SECTION("Silence") {
        auto silence = SignalGenerator::createSilence<float>(signalLength);
        std::vector<std::vector<float>> silenceData { silence, silence };
        SignalAdapterStdVecVec silenceSignal(silenceData);
        
        // check always has to return false
        REQUIRE_FALSE(check<HasSignalInAllBands>(silenceSignal, {}, Freqs{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllBands>(silenceSignal, {1}, Freqs{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllBands>(silenceSignal, {2}, Freqs{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllBands>(silenceSignal, {2}, Freqs{1}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllBands>(silenceSignal, {2}, Freqs{sampleRate/2}, sampleRate));
        
        // check always has to return true - silence is always legal
        REQUIRE(check<HasSignalOnlyInBands>(silenceSignal, {}, Freqs{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(silenceSignal, {1}, Freqs{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(silenceSignal, {2}, Freqs{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(silenceSignal, {2}, Freqs{1}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(silenceSignal, {2}, Freqs{sampleRate/2}, sampleRate));
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
        REQUIRE_FALSE(check<HasSignalInAllBands>(sine, {1}, Freqs{}, sampleRate)); // no frequency is always false
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{}, sampleRate)); // no frequency is always false
        REQUIRE(check<HasSignalInAllBands>(sine, {1}, Freqs{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{1000}, sampleRate));
        REQUIRE(check<HasSignalInAllBands>(sine, {1}, Freqs{1007}, sampleRate)); // +7Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{1007}, sampleRate)); // +7Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{1008}, sampleRate)); // +8Hz is outside bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{{900, 1100}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{2000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{900}, sampleRate));
        
        // 2kHz sine
        REQUIRE(check<HasSignalInAllBands>(sine, {2}, Freqs{2000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(sine, {2}, Freqs{2000}, sampleRate));
        REQUIRE(check<HasSignalInAllBands>(sine, {2}, Freqs{2014}, sampleRate)); // +14Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInBands>(sine, {2}, Freqs{2014}, sampleRate)); // +14Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {2}, Freqs{2016}, sampleRate)); // +16Hz is outside bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInBands>(sine, {2}, Freqs{{1950, 2050}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {2}, Freqs{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {2}, Freqs{2050}, sampleRate)); // 50 Hz is enough to be in next bin at this fs
        
        // multichannel: 1k in L and 2k in R
        REQUIRE(check<HasSignalInAllBands>(sine, {1, 2}, Freqs{{1000, 2000}}, sampleRate, -2)); // both channels are within this range
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1, 2}, Freqs{{1000, 2000}}, sampleRate, -2)); // both channels are within this range
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1, 2}, Freqs{{900, 1500}}, sampleRate)); // R is outside this range
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1, 2}, Freqs{{20, 900}}, sampleRate)); // No signal
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1, 2}, Freqs{{2100, sampleRate/2}}, sampleRate)); // No signal
        
        REQUIRE(check<HasSignalOnlyInBands>(sine, {}, Freqs{{1000, 6000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {}, Freqs{{2000, 6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(sine, {2, 3}, Freqs{{2000, 6000}}, sampleRate));
        
        // set specific threshold
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{1000}, sampleRate, -5.f));
        REQUIRE(check<HasSignalOnlyInBands>(sine, {2}, Freqs{2000}, sampleRate, -5.f));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1}, Freqs{1000}, sampleRate, -12.f));
        REQUIRE(check<HasSignalInAllBands>(sine, {1}, Freqs{500}, sampleRate, -120)); // super low threshold always true
        REQUIRE_FALSE(check<HasSignalInAllBands>(sine, {1}, Freqs{1000}, sampleRate, +.0001f)); // positive threshold cannot be true.
    }
    
    SECTION("Multiple Frequency Ranges") {
        // Sine has 1000, 2000 and 6000 components in channels 1,2,3 respectively
        REQUIRE(check<HasSignalOnlyInBands>(sine, {}, Freqs{{1000}, {2000}, {6000}}, sampleRate)); // All channels have signals in valid ranges
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {}, Freqs{{1000}, {6000}}, sampleRate)); // 2000 Hz is not valid, check in channel 2 fails
        REQUIRE_FALSE(check<HasSignalInAllBands>(sine, {}, Freqs{{1000}, {6000}}, sampleRate)); // one range fails for ch 1 and 3, both fail for ch 2
        REQUIRE(check<HasSignalOnlyInBands>(sine, {}, Freqs{{500}, {1000}, {2000}, {6000}}, sampleRate)); // 500 Hz is not present in signal
        REQUIRE_FALSE(check<HasSignalInAllBands>(sine, {}, Freqs{{500}, {1000}, {2000}, {6000}}, sampleRate)); // 500 Hz is not present in signal
        
        // exclude channel 2
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1, 3}, Freqs{{1000}, {6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1, 3}, Freqs{{1000, 6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInBands>(sine, {1, 3}, Freqs{{600, 1200}, {2100, 10000}}, sampleRate));
        
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {1, 2}, Freqs{{600, 1200}, {2100, 10000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sine, {}, Freqs{{600, 1200}, {2100, 10000}}, sampleRate));
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
        
        REQUIRE(check<HasSignalInAllBands>(sines, {}, Freqs{{1000},{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllBands>(sines, {}, Freqs{{1000}}, sampleRate));
        REQUIRE(check<HasSignalInAllBands>(sines, {}, Freqs{{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllBands>(sines, {}, Freqs{{900,1100},{1800,2200}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllBands>(sines, {}, Freqs{{1050,1100},{2000}}, sampleRate)); // 1k is no longer in range

        REQUIRE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1000},{2000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{2000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1050,1100},{2000}}, sampleRate));
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
        
        REQUIRE_FALSE(check<HasSignalInAllBands>(sines, {}, Freqs{{1000},{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllBands>(sines, {}, Freqs{{1000},{2000}}, sampleRate, thrs));
        REQUIRE(check<HasSignalInAllBands>(sines, {}, Freqs{{1000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInAllBands>(sines, {}, Freqs{{2000}}, sampleRate));
        REQUIRE(check<HasSignalInAllBands>(sines, {}, Freqs{{2000}}, sampleRate, thrs));

        REQUIRE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1000},{2000}}, sampleRate));
        if (thrs > -20.f) { // for lower thresholds, noise gets too high
            REQUIRE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1000},{2000}}, sampleRate, thrs));
        }
        REQUIRE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1000}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{2000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{2000}}, sampleRate, thrs)); // still fale because of the 1k signal
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(sines, {}, Freqs{{1050,1100},{2000}}, sampleRate));
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
            REQUIRE(check<HasSignalInAllBands>(longNoise, {}, Freqs{{1, sampleRate/2}}, sampleRate));
            
            // second bin at 2*ceil(48000/4096)=24Hz
            REQUIRE(sampleRate == 48000);
            REQUIRE_FALSE(check<HasSignalOnlyInBands>(longNoise, {}, Freqs{{24, sampleRate/2}}, sampleRate));
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
            REQUIRE_FALSE(check<HasSignalInAllBands>(noise, {}, Freqs{{20, sampleRate/2}}, sampleRate, +.5f));
            
            // Other than the case above, white noise should always result in true checks
            REQUIRE(check<HasSignalInAllBands>(noise, {}, Freqs{1000}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllBands>(noise, {}, Freqs{10000}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllBands>(noise, {}, Freqs{100}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllBands>(noise, {}, Freqs{sampleRate/2}, sampleRate, thrs));
            REQUIRE(check<HasSignalInAllBands>(noise, {}, Freqs{{1000, sampleRate/2}}, sampleRate, thrs));
            
            // maximum possible frequency range
            REQUIRE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{1, sampleRate/2}}, sampleRate));
            // Other than the case above, white noise should always result in false checks
            REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{1000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{10000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{100}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{sampleRate/2}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{1000, sampleRate/2}}, sampleRate));
        }
    }
    
    SECTION("Band-limited Noise (filtered)")
    {
        AudioFile<float> bandlimitedNoise(std::string(TOSTRING(SOURCE_DIR)) + "/test/test_data/BandLimitedNoise_1k_4k.wav");
        SLB_ASSERT(bandlimitedNoise.getSampleRate() == sampleRate);

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
        
        REQUIRE(check<HasSignalInAllBands>(noise, {}, Freqs{{lowerFreq, upperFreq}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalInAllBands>(noise, {}, Freqs{{20, 400}, {lowerFreq, upperFreq}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalInAllBands>(noise, {}, Freqs{{lowerFreq, upperFreq}, {10000, sampleRate/2}}, sampleRate, thrs));

        REQUIRE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq, upperFreq}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalOnlyAbove>(noise, {}, centerPoint, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyAbove>(noise, {}, lowerFreq, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyBelow>(noise, {}, upperFreq, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq*0.8f, upperFreq*1.5f}}, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq*0.9f, upperFreq}}, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq, upperFreq*1.1f}}, sampleRate, thrs));
        
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq*1.1f, upperFreq}}, sampleRate, thrs));
        REQUIRE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq*1.001f, upperFreq}}, sampleRate, thrs));

        REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq, upperFreq*0.9f}}, sampleRate, thrs));
        REQUIRE_FALSE(check<HasSignalOnlyInBands>(noise, {}, Freqs{{lowerFreq*1.1f, upperFreq*0.9f}}, sampleRate, thrs));
    }
}
