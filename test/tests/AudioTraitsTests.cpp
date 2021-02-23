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

using namespace slb;
using namespace AudioTraits;
using namespace TestCommon;

// Dummy implementations for testing
struct DummyTrait
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels)
    {
        // Verify I cannot modify signal in this context
        static_assert(std::is_assignable<decltype(signal.getData()[0][5]), float >::value == false, "Cannot modify audio data");
        static_assert(std::is_assignable<decltype(signal.getData()[0]), float* >::value == false, "Cannnot modify pointers");
        called = true;
        lastSelectedChannels = selectedChannels;
        return true;
    }
    static bool called;
    static std::set<int> lastSelectedChannels;
};
bool DummyTrait::called = false;
std::set<int> DummyTrait::lastSelectedChannels{};

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
    DummyTrait::lastSelectedChannels = {};
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
        REQUIRE_THROWS(check<DummyTrait>(signal, {1, {1, 3}}));
        REQUIRE_FALSE(DummyTrait::called);
    }
    
    SECTION("Empty selection = all channels") {
        SignalAdapterRaw signal(nullptr, 4, 0);
        REQUIRE(check<DummyTrait>(signal, {}));
        REQUIRE(DummyTrait::lastSelectedChannels == std::set<int>{1, 2, 3, 4});
    }
}

/** helper function to scale a vecvec */
void scale(std::vector<std::vector<float>>& input, ChannelSelection channelSelection, float factorLinear)
{
    auto selectedChannels = channelSelection.get();
    for (int ch=0; ch < (int)input.size(); ++ch) {
        if (selectedChannels.find(ch+1) != selectedChannels.end()) {
            std::transform(input[ch].cbegin(), input[ch].cend(), input[ch].begin(), [factorLinear](auto& s) -> float { return s*factorLinear; });
        }
    }
}

// MARK: - Individual Audio Traits

TEST_CASE("AudioTraits::SignalOnChannels Tests")
{
    std::vector<float> dataL = createRandomVector(16, 333);
    std::vector<float> dataR = createRandomVector(16, 666);
    std::vector<float> zeros(16, 0);
    std::vector<std::vector<float>> buffer = { dataL, dataR };
    SignalAdapterStdVecVec signal(buffer);

    SECTION("Full scale signal") {
        REQUIRE(check<SignalOnChannels>(signal, {{1,2}}));
        REQUIRE_FALSE(check<SignalOnChannels>(signal, {1,2}, 3.f)); // positive threshold never reached

        std::vector<std::vector<float>> bufferL = { dataL, zeros };
        SignalAdapterStdVecVec signalLeftOnly(bufferL);
        REQUIRE(check<SignalOnChannels>(signalLeftOnly, {1}));
        REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}));
        REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}, -40));
        REQUIRE_FALSE(check<SignalOnChannels>(signalLeftOnly, {2}, -144));
    }
    SECTION("Reduced signal on one channel") {
        // scale dataR to -40dB
        scale(buffer, {2}, Utils::dB2Linear(-40.f));
        REQUIRE_FALSE(check<SignalOnChannels>(signal, {2}, -40.f));
        REQUIRE_FALSE(check<SignalOnChannels>(signal, {1, 2}, -40.f)); // channel 1 has signal, but channel 2 hasn't ->false
        REQUIRE(check<SignalOnChannels>(signal, {1}, -40.f));
        REQUIRE(check<SignalOnChannels>(signal, {1, 2}, -50.f));
        
        float absmax = std::max(abs(*std::min_element(dataR.begin(), dataR.end())), *std::max_element(dataR.begin(), dataR.end()));
        float pointWhereSignalIsDetected_dB = -40.f + Utils::linear2Db(absmax);
        REQUIRE(check<SignalOnChannels>(signal, {2}, pointWhereSignalIsDetected_dB - 1e-5f)); // - tolerance
    }
    
}
