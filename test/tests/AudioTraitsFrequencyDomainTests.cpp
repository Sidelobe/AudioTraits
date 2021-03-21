//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"
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
    std::string path = resolveTestFile("sin_1000Hz_-3dBFS_0.1s_2ch.wav");
    AudioFile<float> audioFile(path);
//    audioFile.samples.at(0).resize(480);
//    audioFile.samples.at(1).resize(480);
    SignalAdapterAudioFile<float> sine1kHz(audioFile);
    
    float sampleRate = 48e3;
    auto sine1kSignal = createSine<float>(1000, sampleRate, static_cast<int>(sampleRate));
    auto sine2kSignal = createSine<float>(2000, sampleRate, static_cast<int>(sampleRate));
    std::vector<std::vector<float>> sineData {sine1kSignal, sine2kSignal};
    SignalAdapterStdVecVec sineGenerated(sineData);
    
    REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1}, std::set<std::pair<float, float>>{std::make_pair<float,float>(1000, 1000)}, sampleRate));
    REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {2}, std::set<std::pair<float, float>>{std::make_pair<float,float>(2000, 2000)}, sampleRate));
    REQUIRE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, std::set<std::pair<float, float>>{std::make_pair<float,float>(1000, 2000)}, sampleRate));
    
    REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, std::set<std::pair<float, float>>{std::make_pair<float,float>(20, 900)}, sampleRate)); // No signal
    REQUIRE_FALSE(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, std::set<std::pair<float, float>>{std::make_pair<float,float>(2100, 22000)}, sampleRate)); // No signal
    
    REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, std::set<std::pair<float, float>>{std::make_pair<float,float>(-10, 1)}, sampleRate)); // Outside
    REQUIRE_THROWS(check<HasSignalOnlyInFrequencyRanges>(sineGenerated, {1, 2}, std::set<std::pair<float, float>>{std::make_pair<float,float>(25000, 300000)}, sampleRate)); // Outside possible bin range


}
