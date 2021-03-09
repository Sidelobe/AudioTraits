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
static bool check(const ISignal& signal, const ChannelSelection& channelSelection, Is&& ... traitParams)
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
            auto channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
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

/**
 * Evaluates if the signal represents a delayed version of the reference signal by a given amount of samples.
 * Error tolerance can optionally be specified for both amplitude and time.
 *
 * @note: The longer the delay, the shorter signal left to do the comparison on. Therefore, the delay is limited to
 * 80% of the signal length.
 */
struct IsDelayedVersionOf
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, const ISignal& referenceSignal,
                     int delay_samples, float maxAmplitudeError_percent = 0.f, int maxTimeError_samples = 0)
    {
        ASSERT(delay_samples >= 0, "The delay must be positive");
        ASSERT(maxAmplitudeError_percent >= 0 && maxAmplitudeError_percent <= 100.f, "The delay must be positive");
        ASSERT(maxTimeError_samples >= 0 && maxTimeError_samples <= 5, "Time error has to be between 0 and 5 samples");
        ASSERT((static_cast<float>(delay_samples)/signal.getNumSamples()) < .8f, "The delay cannot be longer than 80% of the signal");
        ASSERT(referenceSignal.getNumSamples() >= signal.getNumSamples() - delay_samples, "The reference signal is not long enough");

        auto amplitudeComp = [&](float a, float b) -> bool
        {
            return std::abs(a-b) <= 1e-2f * maxAmplitudeError_percent;
        };
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> channelSignalRef = referenceSignal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based

            // Exact delays vs. approximate delay [Duplication for better readability]
            if (maxTimeError_samples == 0) {
                std::vector<float> zeroPadding(delay_samples, 0);
                std::vector<float> delayedRef(channelSignalRef); // make a copy and delay it
                delayedRef.insert(delayedRef.begin(), zeroPadding.begin(), zeroPadding.end());
                delayedRef.resize(channelSignalRef.size());
                
                if (std::equal(delayedRef.begin(), delayedRef.end(), channelSignal.begin(), amplitudeComp) == false) {
                    return false; // one channel mismatched is enough to fail
                }
            } else {
                bool thisChannelPassed = false;
                const int& error = maxTimeError_samples;
                
                for (int jitteredDelay = delay_samples-error; jitteredDelay <= delay_samples+error; ++jitteredDelay) {
                    std::vector<float> zeroPadding(std::abs(jitteredDelay), 0);
                    std::vector<float> delayedRef;
                    if (jitteredDelay >= 0) {
                        delayedRef = channelSignalRef;
                    } else {
                        // negative delay: we delay the signal instead of the reference
                        delayedRef = channelSignal;
                    }
                    // delay the copy
                    delayedRef.insert(delayedRef.begin(), zeroPadding.begin(), zeroPadding.end());
                    delayedRef.resize(channelSignalRef.size());
                    
                    if (std::equal(delayedRef.begin(), delayedRef.end(), channelSignal.begin(), amplitudeComp)) {
                        thisChannelPassed = true;
                        break;
                    }
                }
            
                if (!thisChannelPassed) {
                    return false; // none of the 'jittered' delay times was a match
                }
            }
        }
        return true;
    }
  
};

} // namespace AudioTraits
} // namespace slb
