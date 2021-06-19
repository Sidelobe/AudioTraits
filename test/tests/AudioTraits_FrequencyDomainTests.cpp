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
    
    SECTION("Invalid Parameters") {
        std::vector<std::vector<float>> empty;
        SignalAdapterStdVecVec emptySignal(empty);
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(emptySignal, {1}, FrequencySelection{}, sampleRate));
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(emptySignal, {1}, FrequencySelection{}, sampleRate));
    }
    
    SECTION("Sine Wave") {
        float gain_dB = GENERATE(0.f, +3.f, -3.f, -50.f);
        int signalLength = static_cast<int>(sampleRate) * 1;
        auto sine1kSignal = SignalGenerator::createSine<float>(1000, sampleRate, signalLength, gain_dB);
        auto sine2kSignal = SignalGenerator::createSine<float>(2000, sampleRate, signalLength, gain_dB);
        std::vector<std::vector<float>> sineData {sine1kSignal, sine2kSignal};
        SignalAdapterStdVecVec sineGenerated(sineData);

        
        SECTION("Single Frequency Ranges") {
            // 1kHz sine
            REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1}, FrequencySelection{1000}, sampleRate));
            REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1}, FrequencySelection{{900, 1100}}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1}, FrequencySelection{2000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1}, FrequencySelection{900}, sampleRate));

            // 2kHz sine
            REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {2}, FrequencySelection{2000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {2}, FrequencySelection{1000}, sampleRate));
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {2}, FrequencySelection{2050}, sampleRate)); // 50 Hz is enough to be in next bin at this fs
            
            // multichannel: 1k in L and 2k in R
            REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, FrequencySelection{{1000, 2000}}, sampleRate)); // both channels are within this range
            REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, FrequencySelection{{900, 1500}}, sampleRate)); // R is outside this range
        }
        
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, FrequencySelection{{20, 900}}, sampleRate)); // No signal
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, FrequencySelection{{2100, 22000}}, sampleRate)); // No signal
        
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, FrequencySelection{{-10, 1}}, sampleRate)); // Outside
        REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, FrequencySelection{{25000, 300000}}, sampleRate)); // Outside possible bin range
        
        // set specific threshold
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1}, FrequencySelection{1000}, sampleRate, -5.f));
        REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {2}, FrequencySelection{2000}, sampleRate, -5.f));
        REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1}, FrequencySelection{1000}, sampleRate, -12.f));
    }
    
    
//    
//    SECTION("")
//       std::string path = resolveTestFile("sin_1000Hz_-3dBFS_0.1s_2ch.wav");
//        AudioFile<float> audioFile(path);
//    //    audioFile.samples.at(0).resize(480);
//    //    audioFile.samples.at(1).resize(480);
//        SignalAdapterAudioFile<float> sine1kHz(audioFile);
        
}

 
