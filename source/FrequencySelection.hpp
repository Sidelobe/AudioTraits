//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <set>
#include <vector>
#include <utility>

#include "Utils.hpp"

namespace slb
{

class FrequencyRange
{
public:
    /** Discrete component (deliberately non-explicit ctor) */
    FrequencyRange(float frequencyComponent) : m_range{std::make_pair(frequencyComponent, frequencyComponent)}
    {
        ASSERT(frequencyComponent > 0 , "invalid frequency component");
    }
    
    /** Range */
    FrequencyRange(float lowerBound, float uppperBound) : m_range{std::make_pair(lowerBound, uppperBound)}
    {
        ASSERT(uppperBound > lowerBound, "invalid range!");
        ASSERT(lowerBound > 0, "invalid lower bound!");
        ASSERT(uppperBound > 0, "invalid upper bound!");
    }
    
    std::set<std::pair<float, float>> get() const { return { m_range.begin(), m_range.end() }; }
    int size() const { return static_cast<int>(m_range.size()); }

private:
    std::vector<std::pair<float, float>> m_range;
};


class FrequencySelection
{
public:
    FrequencySelection(std::initializer_list<FrequencyRange> selectedRanges) : m_selectedRanges(selectedRanges) {}

    std::set<std::pair<float, float>> get() const
    {
        std::set<std::pair<float, float>> result;
        for (auto& item : m_selectedRanges) {
            auto itemSelection = item.get();
            result.insert(itemSelection.begin(), itemSelection.end());
        }
        return result;
    }
    
private:
    std::vector<FrequencyRange> m_selectedRanges;

};


} // namespace slb
