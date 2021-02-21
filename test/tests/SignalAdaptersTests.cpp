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
    std::string path = resolveTestFile("sin_1000Hz_-3dBFS_0.1s.wav");
    AudioFile<float> audioFile(path);
    SignalAdapterAudioFile<float> adaptedAudioFile(audioFile);
    
    REQUIRE(audioFile.getNumChannels() == adaptedAudioFile.getNumChannels());
    REQUIRE(audioFile.getNumSamplesPerChannel() == adaptedAudioFile.getNumSamples());
    
    //REQUIRE(*(float*)audioFile.samples.data() == *adaptedAudioFile.getData()[0]);

    // check if adapter can be constructed from an rvalue (without an AudioFile stack object)
    SignalAdapterAudioFile<float> shorthand(path);
    REQUIRE(audioFile.getNumChannels() == shorthand.getNumChannels());
    REQUIRE(audioFile.getNumSamplesPerChannel() == shorthand.getNumSamples());
    
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
    
    std::vector<float> dataL = createRandomVector(16, 333);
    std::vector<float> dataR = createRandomVector(16, 666);
    std::vector<std::vector<float>> vecvec{dataL, dataR};

    SignalAdapter2DStdVector adaptedVecVec(vecvec);
    
    REQUIRE(adaptedVecVec.getNumChannels() == 2);
    REQUIRE(adaptedVecVec.getNumSamples() == 16);
//    REQUIRE((float*) &(*adaptedVecVec.getData()) == (float*) vecvec.data());
//    REQUIRE((float*) (&adaptedVecVec.getData()[0]) == dataL.data());
//    REQUIRE((float*) (&adaptedVecVec.getData()[1]) == dataR.data());
}
