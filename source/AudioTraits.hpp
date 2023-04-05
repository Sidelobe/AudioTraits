//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//  
//  © 2023 Lorenz Bucher - all rights reserved
//  https://github.com/Sidelobe/AudioTraits

#pragma once

#include <algorithm>
#include <set>
#include <vector>

#include "ChannelSelection.hpp"
#include "FrequencySelection.hpp"
#include "SignalAdapters.hpp"

#include "AudioTraits-FD.hpp"

namespace slb {
namespace AudioTraits {

// MARK: - Infrastructure
template<typename F, typename ... Is>
static bool check(const ISignal& signal, const ChannelSelection& channelSelection, Is&& ... traitParams)
{
    SLB_ASSERT(signal.getNumSamples() > 0);
    std::set<int> selectedChannels = channelSelection.get();
    SLB_ASSERT(selectedChannels.size() <= signal.getNumChannels());
    std::for_each(selectedChannels.begin(), selectedChannels.end(), [&signal](auto& i) { SLB_ASSERT(i<=signal.getNumChannels()); });

    // Empty selection means all channels
    if (selectedChannels.empty()) {
        std::set<int>::iterator it = selectedChannels.end();
        for (int i=1; i <= signal.getNumChannels(); ++i) {
           it = selectedChannels.insert(it, i);
        }
    }
    
    return F::eval(signal, selectedChannels, std::forward<decltype(traitParams)>(traitParams)...);
}

/** @returns true if a >= b (taking into account tolerance [dB]) */
static inline bool areVectorsEqual(const std::vector<float>& a, const std::vector<float>& b, float tolerance_dB)
{
    SLB_ASSERT(a.size() == b.size(), "Vectors must be of equal length for comparison");
    return std::equal(a.begin(), a.end(), b.begin(), [&tolerance_dB](float v1, float v2)
    {
        float error = std::abs(Utils::linear2Db(std::abs(v1)) - Utils::linear2Db(std::abs(v2)));
        return error <= tolerance_dB;
    });
};


// MARK: - Audio Traits

/**
 * Evaluates if all of the selected channels have at least one sample above the threshold (absolute value)
 */
struct HasSignalOnAllChannels
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
 *
 * Optionally, error tolerance can be specified for both amplitude (in dB [power]) and time (in samples).
 *
 * @note: The longer the delay, the shorter signal left to do the comparison on. Therefore, the delay time is limited
 * to a maximum of 80% of the total signal length.
 */
struct IsDelayedVersionOf
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, const ISignal& referenceSignal,
                     int delay_samples, float amplitudeTolerance_dB = 0.f, int timeTolerance_samples = 0)
    {
        SLB_ASSERT(delay_samples >= 0, "The delay must be positive");
        SLB_ASSERT(amplitudeTolerance_dB >= 0 && amplitudeTolerance_dB < 96.f, "Invalid amplitude tolerance");
        SLB_ASSERT(timeTolerance_samples >= 0 && timeTolerance_samples <= 5, "Time tolerance has to be between 0 and 5 samples");
        SLB_ASSERT((static_cast<float>(delay_samples)/signal.getNumSamples()) < .8f, "The delay cannot be longer than 80% of the signal");
        SLB_ASSERT(referenceSignal.getNumSamples() >= signal.getNumSamples() - delay_samples, "The reference signal is not long enough");

        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> channelSignalRef = referenceSignal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based

            bool thisChannelPassed = false;
            
            // Allow for some tolerance on the delay time: ±maxTimeError_samples
            // Try to match signal with all delay values in this range
            const int& error = timeTolerance_samples;
            for (int jitteredDelay = delay_samples - error; jitteredDelay <= delay_samples + error; ++jitteredDelay) {
                std::vector<float> delayedRef;
                if (jitteredDelay < 0) {
                    // negative delay: we delay the signal instead of the reference
                    delayedRef = channelSignal;
                } else {
                    delayedRef = channelSignalRef;
                }
                // delay the copy we made
                std::vector<float> zeroPadding(std::abs(jitteredDelay), 0);
                delayedRef.insert(delayedRef.begin(), zeroPadding.begin(), zeroPadding.end());
                delayedRef.resize(channelSignal.size());
                
                if (areVectorsEqual(channelSignal, delayedRef, amplitudeTolerance_dB)) {
                    thisChannelPassed = true; // We found a match for this channel
                    break;
                }
            }
            if (!thisChannelPassed) {
                return false; // none of the 'jittered' delay times was a match
            }
        }
        return true;
    }
  
};

/**
 * Evaluates if the signal has matching channels for the entire supplied selection. The matchiing is done on a
 * sample-by-sample basis.
 *
 * Optionally, error tolerance for the matching can be specified in dB
 */
struct HasIdenticalChannels
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float tolerance_dB = 0.f)
    {
        SLB_ASSERT(tolerance_dB >= 0 && tolerance_dB < 96.f, "Invalid amplitude tolerance");
        
        bool doAllChannelsMatch = true;
        std::vector<float> reference(0); // init with size 0
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            if (reference.empty()) {
                reference = channelSignal; // Take first channel as reference
                continue; // no comparison with itself
            }
            doAllChannelsMatch = areVectorsEqual(channelSignal, reference, tolerance_dB);
        }
        
        return doAllChannelsMatch;
    }
  
};

/**
 * Evaluates if two signals have matching channels for the entire supplied selection. The matchiing is done on a
 * sample-by-sample basis.
 *
 * Optionally, error tolerance for the matching can be specified in dB
 */
struct HaveIdenticalChannels
{
    static bool eval(const ISignal& signalA, const std::set<int>& selectedChannels, const ISignal& signalB, float tolerance_dB = 0.f)
    {
        SLB_ASSERT(tolerance_dB >= 0 && tolerance_dB < 96.f, "Invalid amplitude tolerance");
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignalA = signalA.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> channelSignalB = signalB.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            if (!areVectorsEqual(channelSignalA, channelSignalB, tolerance_dB)) {
                return false; // one channel without a match is enough to fail
            }
        }
        return true;
    }
};

} // namespace AudioTraits
} // namespace slb
