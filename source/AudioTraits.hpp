//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <iostream>

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
struct SignalOnChannels
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float threshold_dB = -96.0)
    {
        float threshold_linear = Utils::dB2Linear(threshold_dB);
        
        for (int ch = 0; ch < signal.getNumChannels(); ++ch) {
            int chNumber = ch + 1; // channels are 1-based
            if (selectedChannels.find(chNumber) != selectedChannels.end()) {
                for (int s = 0; s < signal.getNumSamples(); ++s) {
                    if (signal.getData()[ch][s] > threshold_linear) {
                        return true; // one sample above threshold is enough
                    }
                }
            }
        }
        return false;
    }
};



} // namespace AudioTraits
} // namespace slb
