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
static bool check(const ISignal& signal, const ChannelSelection& channelSelection, Is ... i)
{
    // TODO: checks if channel selection is feasible for this signal
    
    return F::eval(signal, channelSelection.get(), std::forward<decltype(i)>(i)...);
}


// MARK: - Audio Traits
struct SignalOnChannels
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float threshold_dB = -96.0)
    {
        std::cout << "Called SignalOnChannels with signal{" << std::to_string(signal.getNumChannels()) << ","
        << std::to_string(signal.getNumSamples()) << "} channel selection=";
        for (auto& ch : selectedChannels) {
            std::cout << std::to_string(ch) << ", ";
        }
        std::cout << " threshold_dB=" << std::to_string(threshold_dB) << std::endl;
        
        float threshold_linear = powf(10, threshold_dB / 20.f);
        
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
