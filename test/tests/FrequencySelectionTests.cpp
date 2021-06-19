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
    REQUIRE_NOTHROW(FrequencyRange(1000));
    REQUIRE_NOTHROW(FrequencyRange{1000});
    REQUIRE_NOTHROW(FrequencyRange(24000));
    REQUIRE_NOTHROW(FrequencyRange(1e10)); // upper bound depends on sampling rate
    REQUIRE_THROWS(FrequencyRange(0));
    REQUIRE_THROWS(FrequencyRange(-10));

    REQUIRE_NOTHROW(FrequencyRange(1000, 1001));
    REQUIRE_NOTHROW(FrequencyRange{1000, 1001});
    REQUIRE_NOTHROW(FrequencyRange(20, 10000));
    
    REQUIRE_THROWS(FrequencyRange(800, 800)); // use single argument ctor for this
    REQUIRE_THROWS(FrequencyRange(1000, 800));
    REQUIRE_THROWS(FrequencyRange(-1, 0));
    REQUIRE_THROWS(FrequencyRange(-100, -10));
    REQUIRE_THROWS(FrequencyRange(0, 0));
}

TEST_CASE("FrequencySelection Tests")
{
    // need to use {} initializers here as soon as we have more than one range
    REQUIRE_NOTHROW(FrequencySelection({20, 300}));
    REQUIRE_NOTHROW(FrequencySelection({1000}));
    REQUIRE_NOTHROW(FrequencySelection({})); // this possible

    using PairSet = std::set<std::pair<float, float>>;
    REQUIRE(FrequencySelection{{20, 300}}.get() == PairSet{std::make_pair(20, 300)});
    REQUIRE(FrequencySelection{3000}.get() == PairSet{std::make_pair(3000, 3000)});
    REQUIRE(FrequencySelection({}).get().empty());
    REQUIRE(FrequencySelection{1000, {20, 300}}.get() == PairSet{{20, 300}, std::make_pair(1000, 1000)});
    
    // duplication
    REQUIRE(FrequencySelection{1000, 3000, 1000}.get() == PairSet{std::make_pair(1000, 1000), std::make_pair(3000, 3000)});
    REQUIRE(FrequencySelection{1000, {1000, 1500}, 1000}.get() == PairSet{std::make_pair(1000, 1000), std::make_pair(1000, 1500)});
}
