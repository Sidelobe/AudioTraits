//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"

#include "ChannelSelection.hpp"

using namespace slb;

TEST_CASE("SelectionItem Tests")
{
    REQUIRE_NOTHROW(SelectionItem(1));
    REQUIRE(SelectionItem(1).size() == 1);
    REQUIRE_NOTHROW(SelectionItem(6));
    REQUIRE(SelectionItem(6).size() == 1);
    
    REQUIRE_NOTHROW(SelectionItem(1, 2));
    REQUIRE(SelectionItem(1, 2).size() == 2);
    REQUIRE(SelectionItem(1, 2).get() == std::set<int>{1, 2});
    REQUIRE_NOTHROW(SelectionItem(4, 6));
    REQUIRE(SelectionItem(4, 6).size() == 3);
    REQUIRE(SelectionItem(4, 6).get() == std::set<int>{4, 5, 6});
    
    // range of size 1 is valid
    REQUIRE_NOTHROW(SelectionItem(6, 6));
    REQUIRE(SelectionItem(6, 6).size() == 1);
    REQUIRE(SelectionItem(6, 6).get() == std::set<int>{6});
    
    // invalid items
    REQUIRE_THROWS(SelectionItem(0));
    REQUIRE_THROWS(SelectionItem(0, 2));
    REQUIRE_THROWS(SelectionItem(2, 1));
}

TEST_CASE("ChannelSelection Tests")
{
    // need to use {} initializers here! ChannelSelection(2, 3) does not compile
    REQUIRE_NOTHROW(ChannelSelection{2, 3});

    REQUIRE(ChannelSelection({2, 3}).get() == std::set<int>{2, 3});
    REQUIRE(ChannelSelection{1}.get() == std::set<int>{1});
    
    REQUIRE(ChannelSelection{4, 7}.get() == std::set<int>{4, 7}); // 2 items
    REQUIRE(ChannelSelection{{4, 7}}.get() == std::set<int>{4, 5, 6, 7}); // 1 item
    
    REQUIRE(ChannelSelection{4, 4}.get() == std::set<int>{4}); // same item twice
    REQUIRE(ChannelSelection{4, 1, 4}.get() == std::set<int>{1, 4}); // same item twice

    // mixed discrete & range
    REQUIRE(ChannelSelection{1, 2, {4, 7}}.get() == std::set<int>{1, 2, 4, 5, 6, 7});
    REQUIRE(ChannelSelection{{4, 7}, 2, 1}.get() == std::set<int>{1, 2, 4, 5, 6, 7});
    REQUIRE(ChannelSelection{1, 2, {4, 7}, 9, 10}.get() == std::set<int>{1, 2, 4, 5, 6, 7, 9, 10});
    
    // redundancy
    REQUIRE(ChannelSelection{1, 2, {4, 7}, 9, 10, 2}.get() == std::set<int>{1, 2, 4, 5, 6, 7, 9, 10});
    REQUIRE(ChannelSelection{1, 4, {4, 7}, 9, 2, 10, 6}.get() == std::set<int>{1, 2, 4, 5, 6, 7, 9, 10});

    // this is valid: it's two items out of order
    REQUIRE_NOTHROW(ChannelSelection{2, 1});
    REQUIRE(ChannelSelection{2, 1}.get() == ChannelSelection{1, 2}.get());

    // invalid selections
    REQUIRE_THROWS(ChannelSelection{0});
    REQUIRE_THROWS(ChannelSelection{-1});
    REQUIRE_THROWS(ChannelSelection{0, 1});
    REQUIRE_THROWS(ChannelSelection{{0, 1}, 4});
    REQUIRE_THROWS(ChannelSelection{{1, 3}, 0});
    REQUIRE_THROWS(ChannelSelection{1, {2, 4}, 0});
}
