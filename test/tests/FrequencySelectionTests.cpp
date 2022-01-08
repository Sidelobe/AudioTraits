//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"

#include "FrequencySelection.hpp"

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

TEST_CASE("FrequencySelection Tests")
{
    // need to use {} initializers here as soon as we have more than one range
    REQUIRE_NOTHROW(FrequencySelection({20, 300}));
    REQUIRE_NOTHROW(FrequencySelection({1000}));
    REQUIRE_NOTHROW(FrequencySelection({})); // this possible

    using PairSet = std::set<std::pair<float, float>>;
    REQUIRE(FrequencySelection{{20, 300}}.getBounds() == PairSet{std::make_pair(20, 300)});
    REQUIRE(FrequencySelection{3000}.getBounds() == PairSet{std::make_pair(3000, 3000)});
    REQUIRE(FrequencySelection({}).getBounds().empty());
    REQUIRE(FrequencySelection{1000, {20, 300}}.getBounds() == PairSet{{20, 300}, std::make_pair(1000, 1000)});
    
    // duplication
    REQUIRE(FrequencySelection{1000, 3000, 1000}.getBounds() == PairSet{std::make_pair(1000, 1000), std::make_pair(3000, 3000)});
    REQUIRE(FrequencySelection{1000, {1000, 1500}, 1000}.getBounds() == PairSet{std::make_pair(1000, 1000), std::make_pair(1000, 1500)});
}
