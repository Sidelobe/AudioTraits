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


TEST_CASE("AudioTraits::HasSignalInFrequencyRanges and HasSignalOnlyInFrequencyRanges tests")
{
    float sampleRate = 48e3f;
    int signalLength = static_cast<int>(sampleRate) * 1;
    
    SECTION("Invalid Parameters") {
        std::vector<std::vector<float>> empty;
        SignalAdapterStdVecVec emptySignal(empty);

        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(emptySignal, {}, FrequencySelection{}, sampleRate));
        
        std::vector<std::vector<float>> minimal(1, std::vector<float>(1));
        SignalAdapterStdVecVec minSig(minimal);
        
        REQUIRE_THROWS(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{0}, sampleRate)); // DC is not valid
        REQUIRE_THROWS(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{-10}, sampleRate)); // negative frequency is not valid
        REQUIRE_THROWS(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{{-10, 200}}, sampleRate)); // range that starts in negative is also invalid
        REQUIRE_THROWS(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2+1}, sampleRate)); // above Nyquist is invalid
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{1}, sampleRate)); // 1Hz is ok
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2}, sampleRate)); // Nyquist is ok
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{}, sampleRate)); // no frequency is always false
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(minSig, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(minSig, {}, FrequencySelection{1000}, sampleRate));
        
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{0}, sampleRate)); // DC is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{-10}, sampleRate)); // negative frequency is not valid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{{-10, 200}}, sampleRate)); // range that starts in negative is also invalid
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2+1}, sampleRate)); // above Nyquist is invalid
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{1}, sampleRate)); // 1Hz is ok
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{sampleRate/2}, sampleRate)); // Nyquist is ok
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{}, sampleRate)); // no frequency is always false
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minSig, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(minSig, {}, FrequencySelection{1000}, sampleRate));
    }
    
    SECTION("Silence") {
        auto silence = SignalGenerator::createSilence<float>(signalLength);
        std::vector<std::vector<float>> silenceData { silence, silence };
        SignalAdapterStdVecVec silenceSignal(silenceData);
        
        // check always has to return false
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(silenceSignal, {}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(silenceSignal, {1}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1000}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{1}, sampleRate));
        REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(silenceSignal, {2}, FrequencySelection{sampleRate/2}, sampleRate));
        
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

        REQUIRE(sampleRate == 48000);
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
            
            REQUIRE_FALSE(check<HasSignalInFrequencyRanges>(longNoise, {}, FrequencySelection{{1, sampleRate/2}}, sampleRate));

            // second bin at 2*ceil(48000/4096)=24Hz
            REQUIRE(sampleRate == 48000);
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(longNoise, {}, FrequencySelection{{24, sampleRate/2}}, sampleRate));
        }
        
        SECTION("Normal cases") {
            float gain_dB = GENERATE(0.f, +3.f, -3.f, -50.f);
            signalLength = static_cast<int>(sampleRate) * 10; // need longer signal
            auto noiseSignal = SignalGenerator::createWhiteNoise(signalLength, gain_dB);
            std::vector<std::vector<float>> noiseData {noiseSignal};
            SignalAdapterStdVecVec noise(noiseData);
            
            // lower threshold because signal is no long enough
            float threshold = -3.f;
            
            // maximum possible frequency range
            REQUIRE(check<HasSignalInFrequencyRanges>(noise, {}, FrequencySelection{{1, sampleRate/2}}, sampleRate, threshold));
            // Other than the case above, white noise should always result in true checks
            REQUIRE(check<HasSignalInFrequencyRanges>(noise, {}, FrequencySelection{1000}, sampleRate, threshold));
            REQUIRE(check<HasSignalInFrequencyRanges>(noise, {}, FrequencySelection{10000}, sampleRate, threshold));
            REQUIRE(check<HasSignalInFrequencyRanges>(noise, {}, FrequencySelection{100}, sampleRate, threshold));
            REQUIRE(check<HasSignalInFrequencyRanges>(noise, {}, FrequencySelection{sampleRate/2}, sampleRate, threshold));
            REQUIRE(check<HasSignalInFrequencyRanges>(noise, {}, FrequencySelection{{1000, sampleRate/2}}, sampleRate, threshold));
            
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

        const float gain_dB = 0.f;//GENERATE(0.f, +3.f, -3.f, -50.f);
        constexpr float lowerFreq = 1000;
        constexpr float upperFreq = 4000;
        REQUIRE(upperFreq-lowerFreq > 100); // test build on this
        constexpr float centerPoint = lowerFreq + (upperFreq-lowerFreq)/2;
        
        SignalAdapterStdVecVec noise(bandlimitedNoise.samples);

        REQUIRE(check<HasSignalInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq}}, sampleRate, -12.0f));
        
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq}}, sampleRate));
        REQUIRE_FALSE(check<HasSignalOnlyAbove>(noise, {}, centerPoint, sampleRate));
        REQUIRE(check<HasSignalOnlyAbove>(noise, {}, lowerFreq, sampleRate));
        REQUIRE(check<HasSignalOnlyBelow>(noise, {}, upperFreq, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*0.8f, upperFreq*1.5f}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*0.9f, upperFreq}}, sampleRate));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq*1.1f}}, sampleRate));
        
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*1.1f, upperFreq}}, sampleRate, -6));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*1.001f, upperFreq}}, sampleRate, -6));

        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq, upperFreq*0.9f}}, sampleRate, -6));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(noise, {}, FrequencySelection{{lowerFreq*1.1f, upperFreq*0.9f}}, sampleRate, -6));
    }
}
