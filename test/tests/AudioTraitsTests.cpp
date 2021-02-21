//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"

#include <vector>

#include "AudioTraits.hpp"

using namespace slb::AudioTraits;
using namespace TestCommon;

TEST_CASE("AudioTraits::SignalOnChannels Tests")
{
    std::vector<float> dataL = createRandomVector(16, 333);
    std::vector<float> dataR = createRandomVector(16, 666);
    std::vector<float> zeros(16, 0);

    float* rawBuffer[] = { dataL.data(), dataR.data() };

    SignalAdapterRaw signal(rawBuffer, 2u, dataL.size());
    
    check<SignalOnChannels>(signal, {1, {3,5}}, 3);
    
    float* rawBufferL[] = { dataL.data(), zeros.data() };
    SignalAdapterRaw signalLeftOnly(rawBufferL, 2u, dataL.size());
    REQUIRE(check<SignalOnChannels>(signalLeftOnly, {1}));
    REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}));
    REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}, -40));
    REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}, -144));
}
