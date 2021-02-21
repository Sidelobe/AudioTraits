//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"

#include "SignalAdapters.hpp"
#include "AudioFileSignalAdapter.hpp"

using namespace TestCommon;

TEST_CASE("SignalAdapters Test AudioFile Adapter")
{
    std::string path = resolveTestFile("sin_1000Hz_-3dBFS_0.1s_2ch.wav");
    AudioFile<float> audioFile(path);
    SignalAdapterAudioFile<float> adaptedAudioFile(audioFile);
    REQUIRE(audioFile.getNumChannels() == 2);
    REQUIRE(audioFile.getNumChannels() == adaptedAudioFile.getNumChannels());
    REQUIRE(audioFile.getNumSamplesPerChannel() == adaptedAudioFile.getNumSamples());
    
    // Ensure that the adapter's data points to the AudioFile object's data
    REQUIRE(audioFile.samples.data()[0].data() == &adaptedAudioFile.getData()[0][0]);
    REQUIRE(audioFile.samples.data()[1].data() == &adaptedAudioFile.getData()[1][0]);

    // check that adapter cannot be constructed from an rvalue (without an AudioFile stack object)
    static_assert(std::is_constructible<SignalAdapterAudioFile<float>, AudioFile<float>>::value == false, "cannot construct from an AudioFile rvalue)");
    static_assert(std::is_constructible<SignalAdapterAudioFile<float>, std::string>::value == false, "cannot construct from a string (implicitly AudioFile rvalue)");
}

TEST_CASE("SignalAdapters Test Raw Adapter")
{
    using namespace slb::AudioTraits;
    
    std::vector<float> dataL = createRandomVector(16, 333);
    std::vector<float> dataR = createRandomVector(16, 666);
    float* rawBuffer[] = { dataL.data(), dataR.data() };

    SignalAdapterRaw adaptedRaw(rawBuffer, 2u, dataL.size());
    
    REQUIRE(adaptedRaw.getNumChannels() == 2);
    REQUIRE(adaptedRaw.getNumSamples() == 16);
    REQUIRE(adaptedRaw.getData() == rawBuffer);
}

TEST_CASE("SignalAdapters Test std::vector<vector>> Adapter")
{
    using namespace slb::AudioTraits;
    
    std::vector<std::vector<float>> vecvec{createRandomVector(16, 333), createRandomVector(16, 666)};

    SignalAdapter2DStdVector adaptedVecVec(vecvec);
    
    REQUIRE(adaptedVecVec.getNumChannels() == 2);
    REQUIRE(adaptedVecVec.getNumSamples() == 16);
    
    // Ensure that the adapter's data points to the wrapped object's data
    REQUIRE(vecvec.data()[0].data() == &adaptedVecVec.getData()[0][0]);
    REQUIRE(vecvec.data()[1].data() == &adaptedVecVec.getData()[1][0]);

    // check that adapter cannot be constructed from an rvalue (without a stack object)
    static_assert(std::is_constructible<SignalAdapter2DStdVector, std::vector<std::vector<float>>>::value == false, "cannot construct from a vector<vector> r-value!");
}
