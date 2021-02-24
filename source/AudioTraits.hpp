//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <algorithm>

#include "ChannelSelection.hpp"
#include "SignalAdapters.hpp"

namespace slb {
namespace AudioTraits {

// MARK: - Infrastructure
template<typename F, typename ... Is>
static bool check(const ISignal& signal, const ChannelSelection& channelSelection, Is ... traitParams)
{
    std::set<int> selectedChannels = channelSelection.get();
    ASSERT(selectedChannels.size() <= signal.getNumChannels());
    std::for_each(selectedChannels.begin(), selectedChannels.end(), [&signal](auto& i) { ASSERT(i<=signal.getNumChannels()); });

    // Empty selection means all channels
    if (selectedChannels.empty()) {
        std::set<int>::iterator it = selectedChannels.end();
        for (int i=1; i <= signal.getNumChannels(); ++i) {
           it = selectedChannels.insert(it, i);
        }
    }
    
    return F::eval(signal, selectedChannels, std::forward<decltype(traitParams)>(traitParams)...);
}


// MARK: - Audio Traits

/**
 * Evaluates if all of the selected channels have at least one sample above the threshold (absolute value)
 */
struct SignalOnAllChannels
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float threshold_dB = -96.f)
    {
        const float threshold_linear = Utils::dB2Linear(threshold_dB);
        for (int chNumber : selectedChannels) {
            auto channelSignal= signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            // find absolute max sample in channel signal
            auto minmax = std::minmax_element(channelSignal.begin(), channelSignal.end());
            float absmax = std::max(std::abs(*std::get<0>(minmax)), *std::get<1>(minmax));
            if (absmax < threshold_linear) {
                return false; // one channel without signal is enough to fail
            }
        }
        return true;
    }
};



} // namespace AudioTraits
} // namespace slb
