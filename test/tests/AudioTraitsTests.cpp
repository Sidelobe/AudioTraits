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

TEST_CASE("AudioTraits Deliberate Failure")
{
    REQUIRE(false);
}


TEST_CASE("AudioTraits Generic Tests")
{
    DummyTrait::called = false;
    DummyTrait::lastSelectedChannels = {};
    SECTION("Empty signal") {
        DummySignal signal(0, 0);
        REQUIRE_THROWS(check<DummyTrait>(signal, {})); // signal cannot have 0 samples
        REQUIRE_FALSE(DummyTrait::called);
        
        DummySignal signal1(0, 1);
        REQUIRE(check<DummyTrait>(signal1, {}));
        REQUIRE(DummyTrait::called);
    }

    SECTION("Illegal channel selections") {
        DummySignal signal(2, 1);
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
        DummySignal signal(4, 1);
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
    std::vector<float> dataL = SignalGenerator::createWhiteNoise(16, 0.f, 333 /*seed*/);
    std::vector<float> dataR = SignalGenerator::createWhiteNoise(16, 0.f, 666 /*seed*/);
    std::vector<float> zeros(16, 0);
    std::vector<std::vector<float>> buffer = { dataL, dataR };
    SignalAdapterStdVecVec signal(buffer);

    SECTION("Full scale signal") {
        REQUIRE(check<HasSignalOnAllChannels>(signal, {{1,2}}));
        REQUIRE_FALSE(check<HasSignalOnAllChannels>(signal, {1,2}, 3.f)); // positive threshold never reached

        std::vector<std::vector<float>> bufferL = { dataL, zeros };
        SignalAdapterStdVecVec signalLeftOnly(bufferL);
        REQUIRE(check<HasSignalOnAllChannels>(signalLeftOnly, {1}));
        REQUIRE_FALSE(check<HasSignalOnAllChannels>(signalLeftOnly, {2}));
        REQUIRE_FALSE(check<HasSignalOnAllChannels>(signalLeftOnly, {2}, -40));
        REQUIRE_FALSE(check<HasSignalOnAllChannels>(signalLeftOnly, {2}, -144));
    }
    SECTION("Reduced signal on one channel") {
        // scale dataR to -40dB
        scale(buffer, {2}, Utils::dB2Linear(-40.f));
        REQUIRE_FALSE(check<HasSignalOnAllChannels>(signal, {2}, -40.f));
        REQUIRE_FALSE(check<HasSignalOnAllChannels>(signal, {1, 2}, -40.f)); // channel 1 has signal, but channel 2 hasn't ->false
        REQUIRE(check<HasSignalOnAllChannels>(signal, {1}, -40.f));
        REQUIRE(check<HasSignalOnAllChannels>(signal, {1, 2}, -50.f));
        
        // Test at the exact detection threshold, taking into account the signal's absmax value
        float absmax = std::max(std::abs(*std::min_element(dataR.begin(), dataR.end())), *std::max_element(dataR.begin(), dataR.end()));
        float pointWhereSignalIsDetected_dB = -40.f + Utils::linear2Db(absmax);
        REQUIRE(check<HasSignalOnAllChannels>(signal, {2}, pointWhereSignalIsDetected_dB - 1e-5f)); // - 'tolerance'
    }
    
}

TEST_CASE("AudioTraits::IsDelayedVersionOf Tests")
{
    int delay = GENERATE(1, 8, 32, 50);
    std::vector<float> dirac = SignalGenerator::createDirac<float>(64);
    std::vector<float> zeros(delay, 0);
    std::vector<float> diracDelayed = SignalGenerator::createDirac<float>(64-delay);
    diracDelayed.insert(diracDelayed.begin(), zeros.begin(), zeros.end());
    REQUIRE(diracDelayed.size() == 64);

    std::vector<std::vector<float>> reference = { dirac, dirac };
    SignalAdapterStdVecVec referenceSignal(reference);
    std::vector<std::vector<float>> buffer = { dirac, diracDelayed };
    SignalAdapterStdVecVec signal(buffer);
    
    SECTION("check signal lengths") {
        dirac.resize(dirac.size()-1);
        std::vector<std::vector<float>> bufferTooShort = { dirac, dirac };
        SignalAdapterStdVecVec bufferTooShortSignal(bufferTooShort);
        REQUIRE_NOTHROW(check<IsDelayedVersionOf>(bufferTooShortSignal, {1}, signal, 0)); // buffer can be shorter than reference
        REQUIRE_THROWS(check<IsDelayedVersionOf>(signal, {1}, bufferTooShortSignal, 0)); // reference is too short
        REQUIRE_NOTHROW(check<IsDelayedVersionOf>(signal, {1}, bufferTooShortSignal, 1)); // delay of 1 -> reference is long enough now
        REQUIRE_THROWS(check<IsDelayedVersionOf>(signal, {1}, referenceSignal, 52)); // delay more than 80%
    }
    
    SECTION("check for both channels") {
        REQUIRE(check<IsDelayedVersionOf>(signal, {1}, referenceSignal, 0));
        REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {1}, referenceSignal, 1));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay-1));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay+1));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {1, 2}, referenceSignal, 0));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {1, 2}, referenceSignal, delay));
        
        std::vector<std::vector<float>> bothDelayed = { diracDelayed, diracDelayed };
        SignalAdapterStdVecVec signalBothDelayed(bothDelayed);
        REQUIRE(check<IsDelayedVersionOf>(signalBothDelayed, {1, 2}, referenceSignal, delay));
        REQUIRE(check<IsDelayedVersionOf>(signalBothDelayed, {1}, referenceSignal, delay));
        REQUIRE(check<IsDelayedVersionOf>(signalBothDelayed, {2}, referenceSignal, delay));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signalBothDelayed, {1, 2}, referenceSignal, delay-1));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signalBothDelayed, {1, 2}, referenceSignal, delay+1));
    }
    
    SECTION("check with more complex signals") {
        std::vector<float> randomData1 = SignalGenerator::createWhiteNoise(64, 0.f, 111 /*seed*/);
        std::vector<float> randomData2 = SignalGenerator::createWhiteNoise(64, 0.f, 112 /*seed*/);
        std::vector<float> randomData3 = SignalGenerator::createWhiteNoise(64, 0.f, 113 /*seed*/);

        std::vector<std::vector<float>> randomData { randomData1, randomData2, randomData3 };
        SignalAdapterStdVecVec randomSignal(randomData);
        
        SECTION("Delay all channels") {
            std::vector<std::vector<float>> randomDataDelayed = randomData;
            for (auto& data : randomDataDelayed) {
                data.insert(data.begin(), zeros.begin(), zeros.end());
                data.resize(randomData1.size());
            }
            SignalAdapterStdVecVec randomSignalDelayed(randomDataDelayed);
            REQUIRE(check<IsDelayedVersionOf>(randomSignalDelayed, {}, randomSignal, delay));
            REQUIRE(check<IsDelayedVersionOf>(randomSignalDelayed, {1, 2}, randomSignal, delay));
            REQUIRE(check<IsDelayedVersionOf>(randomSignalDelayed, {1}, randomSignal, delay));
            REQUIRE(check<IsDelayedVersionOf>(randomSignalDelayed, {3}, randomSignal, delay));
        }
        SECTION("Delay only first channel") {
            std::vector<std::vector<float>> randomDataDelayed = randomData;
            randomDataDelayed[0].insert(randomDataDelayed[0].begin(), zeros.begin(), zeros.end());
            randomDataDelayed[0].resize(randomData1.size());
            
            SignalAdapterStdVecVec randomSignalDelayed(randomDataDelayed);
            REQUIRE_FALSE(check<IsDelayedVersionOf>(randomSignalDelayed, {}, randomSignal, delay));
            REQUIRE_FALSE(check<IsDelayedVersionOf>(randomSignalDelayed, {1, 2}, randomSignal, delay));
            REQUIRE(check<IsDelayedVersionOf>(randomSignalDelayed, {1}, randomSignal, delay));
            REQUIRE_FALSE(check<IsDelayedVersionOf>(randomSignalDelayed, {3}, randomSignal, delay));
        }
    }
    
    SECTION("Amplitude error") {
        std::vector<std::vector<float>> delayedAndScaled = { diracDelayed, diracDelayed };
        SECTION("Scale both channels") {
            scale(delayedAndScaled, {1, 2}, Utils::dB2Linear(-1.f));
            SignalAdapterStdVecVec signalScaled(delayedAndScaled);
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay)); // won't pass with default
            REQUIRE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay, 1.001f));        // amplitude tolerance 1dB
            REQUIRE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay, 3.001f));        // amplitude tolerance 3dB
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay, 0.9999f)); // amplitude tolerance below 1dB
            
            scale(delayedAndScaled, {1, 2}, Utils::dB2Linear(-2.f));
            SignalAdapterStdVecVec signalScaledMore(delayedAndScaled);
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaledMore, {}, referenceSignal, delay)); // won't pass with default
            REQUIRE(check<IsDelayedVersionOf>(signalScaledMore, {}, referenceSignal, delay, 3.001f));        // amplitude tolerance 1dB
            REQUIRE(check<IsDelayedVersionOf>(signalScaledMore, {}, referenceSignal, delay, 4.001f));        // amplitude tolerance 3dB
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaledMore, {}, referenceSignal, delay, 2.999f)); // amplitude tolerance below 1dB
            
        }
        SECTION("Scale one channel only") {
            scale(delayedAndScaled, {2}, Utils::dB2Linear(-1.f));
            SignalAdapterStdVecVec signalBothDelayedRightScaled(delayedAndScaled);
            REQUIRE(check<IsDelayedVersionOf>(signalBothDelayedRightScaled, {1}, referenceSignal, delay));
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalBothDelayedRightScaled, {2}, referenceSignal, delay)); // won't pass with default
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalBothDelayedRightScaled, {1, 2}, referenceSignal, delay)); // won't pass with default
            REQUIRE(check<IsDelayedVersionOf>(signalBothDelayedRightScaled, {}, referenceSignal, delay, 1.001f));       // amplitude tolerance 1dB
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalBothDelayedRightScaled, {}, referenceSignal, delay, 0.5f)); // amplitude tolerance 0.5dB
        }
    }
    
    SECTION("Time Error") {
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {1}, referenceSignal, delay, 0.f, 0));
        REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay, 0.f, 0));

        REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay,     0.f, 1));
        REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay + 1, 0.f, 1));
        REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay - 1, 0.f, 1));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay + 1, 0.f, 0));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay - 1, 0.f, 0));
        
        // delay
        if (delay > 4 && delay < 50) {
            REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay + 5, 0.f, 5));
            REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay - 5, 0.f, 5));
            REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay + 4, 0.f, 5));
            REQUIRE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay - 4, 0.f, 5));
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay + 6, 0.f, 5));
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {2}, referenceSignal, delay - 6, 0.f, 5));
        }
    }
    
    SECTION("Amplitude & Time Error") {
        std::vector<std::vector<float>> delayedAndScaled = { diracDelayed, diracDelayed };
        scale(delayedAndScaled, {1, 2}, Utils::dB2Linear(-1.f));
        SignalAdapterStdVecVec signalScaled(delayedAndScaled);
        REQUIRE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay, 1.001f, 0));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay, 0.5f)); // fails because of amplitude tolerance
        if (delay > 4 && delay < 50) {
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay +2, 1.001f, 1)); // fails because of delay tolerance
            REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay -2, 1.001f, 1)); // fails because of delay tolerance
        }
        REQUIRE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay + 1, 1.001f, 1));
        REQUIRE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay - 1, 1.001f, 1));
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay + 1, .5f, 1)); // fails because of both
        REQUIRE_FALSE(check<IsDelayedVersionOf>(signalScaled, {}, referenceSignal, delay - 1, .5f, 1)); // fails because of both
    }
    
}

TEST_CASE("AudioTraits::HasIdenticalChannels Tests")
{
    std::vector<float> data1 = SignalGenerator::createWhiteNoise(16, 0.f, 333 /*seed*/);
    std::vector<float> data2 = SignalGenerator::createWhiteNoise(16, 0.f, 666 /*seed*/);
    std::vector<float> zeros(16, 0);
    std::vector<std::vector<float>> buffer = { data1, data1, zeros, data2, zeros, data2 };
    SignalAdapterStdVecVec signal(buffer);
    
    REQUIRE(check<HasIdenticalChannels>(signal, {1})); // one channels channel always matches itself
    REQUIRE(check<HasIdenticalChannels>(signal, {3})); // one channels channel always matches itself
    REQUIRE(check<HasIdenticalChannels>(signal, {4})); // one channels channel always matches itself
    REQUIRE_FALSE(check<HasIdenticalChannels>(signal, {})); // all channels
    REQUIRE(check<HasIdenticalChannels>(signal, {1, 2}));
    REQUIRE(check<HasIdenticalChannels>(signal, {4, 6}));
    REQUIRE_FALSE(check<HasIdenticalChannels>(signal, {1, 4}));
    REQUIRE_FALSE(check<HasIdenticalChannels>(signal, {1, 3}));
    
    scale(buffer, {1}, Utils::dB2Linear(-1.f));
    REQUIRE_FALSE(check<HasIdenticalChannels>(signal, {1, 2}));
    REQUIRE(check<HasIdenticalChannels>(signal, {1, 2}, 1.001f));
    REQUIRE_FALSE(check<HasIdenticalChannels>(signal, {1, 2}, 0.999f));
    
    scale(buffer, {4}, Utils::dB2Linear(-1.f));
    scale(buffer, {6}, Utils::dB2Linear(-4.f));
    REQUIRE_FALSE(check<HasIdenticalChannels>(signal, {4, 6}));
    REQUIRE(check<HasIdenticalChannels>(signal, {4, 6}, 3.001f));
    REQUIRE_FALSE(check<HasIdenticalChannels>(signal, {4, 6}, 2.999f));
}

TEST_CASE("AudioTraits::HaveIdenticalChannels Tests")
{
    std::vector<float> data1A = SignalGenerator::createWhiteNoise(16, 0.f, 333 /*seed*/);
    std::vector<float> data2A = SignalGenerator::createWhiteNoise(16, 0.f, 666 /*seed*/);
    std::vector<float> data1B = SignalGenerator::createWhiteNoise(16, 0.f, 333 /*seed*/);
    std::vector<float> data2B = SignalGenerator::createWhiteNoise(16, 0.f, 666 /*seed*/);
    std::vector<float> zeros(16, 0);
    std::vector<std::vector<float>> bufferA = { data1A, data1A, zeros, data2A, zeros, data2A };
    std::vector<std::vector<float>> bufferB = { data1B, data2B, data1B, data2B, zeros, data2B };
    SignalAdapterStdVecVec signalA(bufferA);
    SignalAdapterStdVecVec signalB(bufferB);

    REQUIRE(check<HaveIdenticalChannels>(signalA, {}, signalA)); // comparison with itself always true
    REQUIRE(check<HaveIdenticalChannels>(signalA, {1}, signalA));
    REQUIRE(check<HaveIdenticalChannels>(signalB, {5}, signalB));
    
    REQUIRE(check<HaveIdenticalChannels>(signalA, {1}, signalB));
    REQUIRE(check<HaveIdenticalChannels>(signalA, {4}, signalB)); // comparison with zero always true
    REQUIRE_FALSE(check<HaveIdenticalChannels>(signalA, {}, signalB)); // fail for all channels
    REQUIRE_FALSE(check<HaveIdenticalChannels>(signalA, {1, 2}, signalB));
    REQUIRE(check<HaveIdenticalChannels>(signalA, {1, 4, 5, 6}, signalB));
    REQUIRE(check<HaveIdenticalChannels>(signalA, {{4, 6}}, signalB));
    REQUIRE(check<HaveIdenticalChannels>(signalA, {1, 4}, signalB));
    REQUIRE_FALSE(check<HaveIdenticalChannels>(signalA, {1, 2, 4, 5}, signalB));

    scale(bufferA, {1, 4, 5}, Utils::dB2Linear(-1.f));
    REQUIRE_FALSE(check<HaveIdenticalChannels>(signalA, {1, 4, 5}, signalB));
    REQUIRE(check<HaveIdenticalChannels>(signalA, {1, 4, 5}, signalB, 1.001f));
    REQUIRE_FALSE(check<HaveIdenticalChannels>(signalA, {1, 4, 5}, signalB, 0.999f));

    scale(bufferA, {6}, Utils::dB2Linear(-1.f));
    scale(bufferB, {1, {4, 6}}, Utils::dB2Linear(-4.f));
    REQUIRE_FALSE(check<HaveIdenticalChannels>(signalA, {1, {4, 6}}, signalB));
    REQUIRE(check<HaveIdenticalChannels>(signalA, {1, {4, 6}}, signalB, 3.001f));
    REQUIRE_FALSE(check<HaveIdenticalChannels>(signalA, {1, {4, 6}}, signalB, 2.999f));
}
