//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//  
//  © 2023 Lorenz Bucher - all rights reserved
//  https://github.com/Sidelobe/AudioTraits

#pragma once

#include <set>
#include <vector>
#include <utility>

#include "Utils.hpp"

namespace slb
{

class FreqBand
{
public:
    using Bounds = std::pair<float, float>;
    
    /**
     * Discrete frequency component (deliberately non-explicit ctor).
     * Treated as a range with equal upper and lower bound.
     */
    FreqBand(float frequencyComponent) : m_range{std::make_pair(frequencyComponent, frequencyComponent)}
    {
        SLB_ASSERT(frequencyComponent > 0 , "invalid frequency component");
    }
    
    /**
     * Range of frequencies (deliberately non-explicit ctor) 
     * TODO: consider making this explicit -- maybe rename class to something shorter in this case
     */
    FreqBand(float lowerBound, float uppperBound) : m_range{std::make_pair(lowerBound, uppperBound)}
    {
        SLB_ASSERT(uppperBound > lowerBound, "invalid range!");
        SLB_ASSERT(lowerBound > 0, "invalid lower bound!");
        SLB_ASSERT(uppperBound > 0, "invalid upper bound!");
    }
    
    Bounds get() const { return m_range; }
    float size() const { return std::abs(std::get<1>(m_range) - std::get<0>(m_range)); }
    float centerFrequency() const { return std::get<0>(m_range) + size()/2; }

private:
    Bounds m_range;
};


class Freqs
{
public:
    explicit Freqs(std::initializer_list<FreqBand> selectedRanges) : m_selectedRanges(selectedRanges) {}
    
    /** @return a duplicate-free list of FrequencyRange Bound pairs contained in the selection */
    std::set<FreqBand::Bounds> getBounds() const
    {
        std::set<FreqBand::Bounds> result;
        for (auto& item : m_selectedRanges) {
            result.insert(item.get());
        }
        return result;
    }
    
    /** @return a copy of all the FrequencyRanges in the selection */
    std::vector<FreqBand> getRanges() const
    {
        return m_selectedRanges;
    }
    
private:
    std::vector<FreqBand> m_selectedRanges;

};


} // namespace slb
