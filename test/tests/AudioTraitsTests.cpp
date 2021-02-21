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

struct DummyTrait
{
    static bool called;
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels)
    {
        UNUSED(signal);
        UNUSED(selectedChannels);
        called = true;
        return true;
    }
};
bool DummyTrait::called = false;

class DummySignal : ISignal
{
public:
    DummySignal(int numChannels, int numSamples) : m_numChannels(numChannels), m_numSamples(numSamples) {}
    int getNumChannels() const override { return m_numChannels; }
    int getNumSamples()  const override { return m_numSamples;  }
    const float* const* getData() const override { return nullptr; }
private:
    const int m_numChannels;
    const int m_numSamples;
};

TEST_CASE("AudioTraits Generic Tests")
{
    DummyTrait::called = false;
    SECTION("Empty signal") {
        SignalAdapterRaw signal(nullptr, 0, 0);
        REQUIRE(check<DummyTrait>(signal, {}));
        REQUIRE(DummyTrait::called);
    }

    SECTION("Illegal channel selections") {
        SignalAdapterRaw signal(nullptr, 2, 0);
        REQUIRE(check<DummyTrait>(signal, {1, 2}));
        REQUIRE(DummyTrait::called); DummyTrait::called = false;
        REQUIRE(check<DummyTrait>(signal, {{1, 2}}));
        REQUIRE(DummyTrait::called); DummyTrait::called = false;
        
        REQUIRE_THROWS(check<DummyTrait>(signal, {1, 2, 3}));
        REQUIRE_THROWS(check<DummyTrait>(signal, {3}));
        REQUIRE_FALSE(DummyTrait::called);
    }
}

TEST_CASE("AudioTraits::SignalOnChannels Tests")
{
    std::vector<float> dataL = createRandomVector(16, 333);
    std::vector<float> dataR = createRandomVector(16, 666);
    std::vector<float> zeros(16, 0);

    float* rawBuffer[] = { dataL.data(), dataR.data() };

    SignalAdapterRaw signal(rawBuffer, 2u, dataL.size());

    REQUIRE(check<SignalOnChannels>(signal, {{1,2}}));
    REQUIRE_FALSE(check<SignalOnChannels>(signal, {1,2}, 3.f)); // positive threshold never reached

    float* rawBufferL[] = { dataL.data(), zeros.data() };
    SignalAdapterRaw signalLeftOnly(rawBufferL, 2u, dataL.size());
    REQUIRE(check<SignalOnChannels>(signalLeftOnly, {1}));
    REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}));
    REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}, -40));
    REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}, -144));
}
