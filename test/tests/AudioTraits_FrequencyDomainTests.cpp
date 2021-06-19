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

using namespace slb;
using namespace AudioTraits;
using namespace TestCommon;

TEST_CASE("AudioTraits::HasSignalOnlyInFrequencyRanges tests")
{
    float sampleRate = 48e3;
    int signalLength = static_cast<int>(sampleRate) * 1;
    
    SECTION("Invalid Parameters") {
        std::vector<std::vector<float>> empty;
        SignalAdapterStdVecVec emptySignal(empty);

        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(emptySignal, {}, FrequencySelection{}, sampleRate));
        
        std::vector<std::vector<float>> minimal(1, std::vector<float>(1));
        SignalAdapterStdVecVec minimalSignal(minimal);
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{0}, sampleRate)); // DC is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{-10}, sampleRate)); // negative frequency is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{{-10, 200}}, sampleRate)); // range that starts in negative is also invalid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{sampleRate/2+1}, sampleRate)); // above Nyquist is invalid
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{1}, sampleRate)); // 1Hz is ok
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{sampleRate/2}, sampleRate)); // Nyquist is ok
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{}, sampleRate)); // no frequency is always false
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minimalSignal, {}, FrequencySelection{1000}, sampleRate));
    }
    
    SECTION("Silence") {
        auto silence = SignalGenerator::createSilence<float>(signalLength);
        std::vector<std::vector<float>> silenceData { silence, silence };
        SignalAdapterStdVecVec silenceSignal(silenceData);
        
        // check always has to return false
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{sampleRate/2}, sampleRate));
    }
    
    SECTION("Sine Waves") {
        float gain_dB = GENERATE(0.f, +3.f, -3.f, -50.f);
        auto sine1kSignal = SignalGenerator::createSine<float>(1000, sampleRate, signalLength, gain_dB);
        auto sine2kSignal = SignalGenerator::createSine<float>(2000, sampleRate, signalLength, gain_dB);
        auto sine6kSignal = SignalGenerator::createSine<float>(6000, sampleRate, signalLength, gain_dB);
        std::vector<std::vector<float>> sineData {sine1kSignal, sine2kSignal, sine6kSignal};
        SignalAdapterStdVecVec sine(sineData);

        // Single Frequency Ranges
        // 1kHz sine
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{}, sampleRate)); // no frequency is always false
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1007}, sampleRate)); // +7Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{1008}, sampleRate)); // +8Hz is outside bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{{900, 1100}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{2000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1}, FrequencySelection{900}, sampleRate));

        // 2kHz sine
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2000}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2014}, sampleRate)); // +14Hz is still within bin (for this freq, fft size & samplerate)
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2016}, sampleRate)); // +16Hz is outside bin (for this freq, fft size & samplerate)
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{{1950, 2050}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {2}, FrequencySelection{2050}, sampleRate)); // 50 Hz is enough to be in next bin at this fs

        // multichannel: 1k in L and 2k in R
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 2}, FrequencySelection{{1000, 2000}}, sampleRate)); // both channels are within this range
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
    

        // Multiple Frequency Ranges
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{1000}, {2000}, {6000}}, sampleRate)); // All channels have signals in valid ranges
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{1000}, {6000}}, sampleRate)); // 2000 Hz is not valid
        // exclude channel 2
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 3}, FrequencySelection{{1000}, {6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 3}, FrequencySelection{{1000, 6000}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 3}, FrequencySelection{{600, 1200}, {2100, 10000}}, sampleRate));
        
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {1, 2}, FrequencySelection{{600, 1200}, {2100, 10000}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sine, {}, FrequencySelection{{600, 1200}, {2100, 10000}}, sampleRate));
    }
    
    SECTION("White Noise (unfiltered)") {
        
        SECTION("Extreme case (Ultra-low frequency, very long signal)") {
            signalLength = static_cast<int>(sampleRate) * 35;
            auto noiseSignal = SignalGenerator::createWhiteNoise(signalLength);
            std::vector<std::vector<float>> longNoiseData {noiseSignal, noiseSignal};
            SignalAdapterStdVecVec longNoise(longNoiseData);
            // second bin at 2*ceil(48000/4096)=24Hz
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(longNoise, {}, FrequencySelection{{24, sampleRate/2}}, sampleRate));
        }
        
        SECTION("Normal cases") {
            float gain_dB = GENERATE(0.f, +3.f, -3.f, -50.f);
            auto noiseSignal = SignalGenerator::createWhiteNoise(signalLength, gain_dB);
            std::vector<std::vector<float>> noiseData {noiseSignal};
            SignalAdapterStdVecVec noise(noiseData);
            
            REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{1, sampleRate/2}}, sampleRate)); // maximum possible frequency range

            // Other than the case above, white noise should always result in false checks
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{1000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{10000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{100}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{sampleRate/2}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{1000, sampleRate/2}}, sampleRate));
        }
    }
    
    // TODO: filtered white noise
}
