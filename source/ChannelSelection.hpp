//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <array>
#include <numeric>
#include <utility>
#include <vector>

#include "Utils.hpp"

namespace slb
{

class SelectionItem
{
public:
    /** Discrete */
    SelectionItem(int channel) : m_selectedChannels({channel})
    {
        ASSERT(channel > 0, "invalid channel!");
    }
    
    /** Range */
    SelectionItem(int first, int last) : m_selectedChannels(last - first + 1)
    {
        ASSERT(first > 0 && last >= first, "invalid range!");
        std::iota(m_selectedChannels.begin(), m_selectedChannels.end(), first);
    }
    
    int size() const { return static_cast<int>(m_selectedChannels.size()); }

private:
    std::vector<int> m_selectedChannels;
};



} // namespace slb
