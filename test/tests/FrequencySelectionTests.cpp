//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//  
//  © 2023 Lorenz Bucher - all rights reserved
//  https://github.com/Sidelobe/AudioTraits

#include "TestCommon.hpp"

#include "AudioTraits.hpp"
//#include "FrequencySelection.hpp"

using namespace slb;

TEST_CASE("FrequencyRange Tests")
{
    REQUIRE_NOTHROW(FreqBand(1000));
    REQUIRE_NOTHROW(FreqBand{1000});
    REQUIRE_NOTHROW(FreqBand(24000));
    REQUIRE_NOTHROW(FreqBand(1e10)); // upper bound depends on sampling rate
    REQUIRE_THROWS(FreqBand(0));
    REQUIRE_THROWS(FreqBand(-10));

    REQUIRE_NOTHROW(FreqBand(1000, 1001));
    REQUIRE_NOTHROW(FreqBand{1000, 1001});
    REQUIRE_NOTHROW(FreqBand(20, 10000));
    
    REQUIRE_THROWS(FreqBand(800, 800)); // use single argument ctor for this
    REQUIRE_THROWS(FreqBand(1000, 800));
    REQUIRE_THROWS(FreqBand(-1, 0));
    REQUIRE_THROWS(FreqBand(-100, -10));
    REQUIRE_THROWS(FreqBand(0, 0));
}

TEST_CASE("Freqs Tests")
{
    // need to use {} initializers here as soon as we have more than one range
    REQUIRE_NOTHROW(Freqs({20, 300}));
    REQUIRE_NOTHROW(Freqs({1000}));
    REQUIRE_NOTHROW(Freqs({})); // this possible

    using PairSet = std::set<std::pair<float, float>>;
    REQUIRE(Freqs{{20, 300}}.getBounds() == PairSet{std::make_pair(20, 300)});
    REQUIRE(Freqs{3000}.getBounds() == PairSet{std::make_pair(3000, 3000)});
    REQUIRE(Freqs({}).getBounds().empty());
    REQUIRE(Freqs{1000, {20, 300}}.getBounds() == PairSet{{20, 300}, std::make_pair(1000, 1000)});
    
    // duplication
    REQUIRE(Freqs{1000, 3000, 1000}.getBounds() == PairSet{std::make_pair(1000, 1000), std::make_pair(3000, 3000)});
    REQUIRE(Freqs{1000, {1000, 1500}, 1000}.getBounds() == PairSet{std::make_pair(1000, 1000), std::make_pair(1000, 1500)});
}
