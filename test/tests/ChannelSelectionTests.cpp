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
    REQUIRE_THROWS(SelectionItem(0));
    
    REQUIRE_NOTHROW(SelectionItem(1, 2));
    REQUIRE(SelectionItem(1, 2).size() == 2);
    REQUIRE_NOTHROW(SelectionItem(4, 6));
    REQUIRE(SelectionItem(4, 6).size() == 3);
    REQUIRE_NOTHROW(SelectionItem(6, 6));
    REQUIRE(SelectionItem(6, 6).size() == 1);
    REQUIRE_THROWS(SelectionItem(0, 2));
    REQUIRE_THROWS(SelectionItem(0, 2));

}
