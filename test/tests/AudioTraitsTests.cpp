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
        static_assert(std::is_assignable<decltype(signal.getChannelDataCopy(0)[0]), float >::value == true, "Can modify copy");
        called = true;
        lastSelectedChannels = selectedChannels;
        return true;
    }
    static bool called;
    static std::set<int> lastSelectedChannels;
};
bool DummyTrait::called = false;
std::set<int> DummyTrait::lastSelectedChannels{};

class DummySignal : public ISignal
{
public:
    DummySignal(int numChannels, int numSamples) : m_numChannels(numChannels), m_numSamples(numSamples) {}
    int getNumChannels() const override { return m_numChannels; }
    int getNumSamples()  const override { return m_numSamples;  }
    const float* const* getData() const override { return nullptr; }
    std::vector<float> getChannelDataCopy(int channelIndex) const override { return {}; UNUSED(channelIndex); }
private:
    const int m_numChannels;
    const int m_numSamples;
};

TEST_CASE("AudioTraits Generic Tests")
{
    DummyTrait::called = false;
    DummyTrait::lastSelectedChannels = {};
    SECTION("Empty signal") {
        DummySignal signal(0, 0);
        REQUIRE(check<DummyTrait>(signal, {}));
        REQUIRE(DummyTrait::called);
    }

    SECTION("Illegal channel selections") {
        DummySignal signal(2, 0);
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
        DummySignal signal(4, 0);
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

TEST_CASE("AudioTraits::SignalOnAllChannels Tests")
{
    std::vector<float> dataL = createRandomVector(16, 333);
    std::vector<float> dataR = createRandomVector(16, 666);
    std::vector<float> zeros(16, 0);
    std::vector<std::vector<float>> buffer = { dataL, dataR };
    SignalAdapterStdVecVec signal(buffer);

    SECTION("Full scale signal") {
        REQUIRE(check<SignalOnAllChannels>(signal, {{1,2}}));
        REQUIRE_FALSE(check<SignalOnAllChannels>(signal, {1,2}, 3.f)); // positive threshold never reached

        std::vector<std::vector<float>> bufferL = { dataL, zeros };
        SignalAdapterStdVecVec signalLeftOnly(bufferL);
        REQUIRE(check<SignalOnAllChannels>(signalLeftOnly, {1}));
        REQUIRE_FALSE(check<SignalOnAllChannels>(signalLeftOnly, {2}));
        REQUIRE_FALSE(check<SignalOnAllChannels>(signalLeftOnly, {2}, -40));
        REQUIRE_FALSE(check<SignalOnAllChannels>(signalLeftOnly, {2}, -144));
    }
    SECTION("Reduced signal on one channel") {
        // scale dataR to -40dB
        scale(buffer, {2}, Utils::dB2Linear(-40.f));
        REQUIRE_FALSE(check<SignalOnAllChannels>(signal, {2}, -40.f));
        REQUIRE_FALSE(check<SignalOnAllChannels>(signal, {1, 2}, -40.f)); // channel 1 has signal, but channel 2 hasn't ->false
        REQUIRE(check<SignalOnAllChannels>(signal, {1}, -40.f));
        REQUIRE(check<SignalOnAllChannels>(signal, {1, 2}, -50.f));
        
        // Test at the exact detection threshold, taking into account the signal's absmax value
        float absmax = std::max(std::abs(*std::min_element(dataR.begin(), dataR.end())), *std::max_element(dataR.begin(), dataR.end()));
        float pointWhereSignalIsDetected_dB = -40.f + Utils::linear2Db(absmax);
        REQUIRE(check<SignalOnAllChannels>(signal, {2}, pointWhereSignalIsDetected_dB - 1e-5f)); // - 'tolerance'
    }
    
}

TEST_CASE("AudioTraits::IsDelayedVersionOf Tests")
{
    std::vector<float> dirac = createDirac<float>(64);
    std::vector<float> diracDelayed8 = createDirac<float>(64-8);
    std::vector<float> zeros(8, 0);
    diracDelayed8.insert(diracDelayed8.begin(), zeros.begin(), zeros.end());
    REQUIRE(diracDelayed8.size() == 64);

    std::vector<std::vector<float>> reference = { dirac, dirac };
    SignalAdapterStdVecVec referenceSignal(reference);
    std::vector<std::vector<float>> buffer = { dirac, diracDelayed8 };
    SignalAdapterStdVecVec signal(buffer);
    
    SECTION("check signal lengths") {
        dirac.resize(dirac.size()-1);
        std::vector<std::vector<float>> bufferTooShort = { dirac, dirac };
        SignalAdapterStdVecVec bufferTooShortSignal(bufferTooShort);
        REQUIRE_NOTHROW(check<IsDelayedVersionOf>(bufferTooShortSignal, {1}, signal, 0)); // buffer can be shorter than reference
        REQUIRE_THROWS(check<IsDelayedVersionOf>(signal, {1}, bufferTooShortSignal, 0)); // reference is too short
        REQUIRE_NOTHROW(check<IsDelayedVersionOf>(signal, {1}, bufferTooShortSignal, 1)); // delay of 1 -> reference is long enough now
    }
    
    SECTION("check for both channels") {
        REQUIRE(check<IsDelayedVersionOf>(signal, {1}, referenceSignal, 0));
        REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, 8));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {1}, referenceSignal, 1));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, 7));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, 9));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {1, 2}, referenceSignal, 0));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {1, 2}, referenceSignal, 8));
        
        std::vector<std::vector<float>> bothDelayed = { diracDelayed8, diracDelayed8 };
        SignalAdapterStdVecVec signalBothDelayed(bothDelayed);
        REQUIRE(check<IsDelayedVersionOf>(signalBothDelayed, {1, 2}, referenceSignal, 8));
        REQUIRE(check<IsDelayedVersionOf>(signalBothDelayed, {1}, referenceSignal, 8));
        REQUIRE(check<IsDelayedVersionOf>(signalBothDelayed, {2}, referenceSignal, 8));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signalBothDelayed, {1, 2}, referenceSignal, 7));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signalBothDelayed, {1, 2}, referenceSignal, 9));
    }
    
}
