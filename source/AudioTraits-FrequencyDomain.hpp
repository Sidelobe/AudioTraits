//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <algorithm>
#include <vector>

#include "ChannelSelection.hpp"
#include "SignalAdapters.hpp"

#include "FrequencySelection.hpp"
#include "FFT/FrequencyDomainHelpers.hpp"

namespace slb {
namespace AudioTraits {

// MARK: - Frequency Domain Audio Traits

/**
 * Evaluates if all the selected channels have frequency content in all the specified ranges.
 *
 * To count as 'there is frequency content', it needs to be above a certain threshold in dB (relative to the maximum bin).
 * If any other frequency ranges (FFT bins) reach this threshold, the result will be 'false'
 *
 * TODO: add option to re-use 'cache' the FFT results instead of recalculating every time.
 */
struct HasSignalInFrequencyRanges
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, FrequencySelection frequencySelection,
                     float sampleRate, float threshold_dB = -0.5f)
    {
        // Determine bins where signal is expected
        std::set<int> expectedBins = FrequencyDomainHelpers::determineCorrespondingBins(frequencySelection, sampleRate);
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> normalizedBins = FrequencyDomainHelpers::getNormalizedFrequencyBins(channelSignal);
            
            float min = *std::min_element(normalizedBins.begin(), normalizedBins.end());
            bool hasValidSignal = false;
            for (int expectedBin : expectedBins) {
                float binValue_dB = Utils::linear2Db(normalizedBins.at(expectedBin));
                if (binValue_dB < threshold_dB) {
                    return false; // there's no signal in at least one bin in the selected range
                }
                hasValidSignal = true;
            }
            if (hasValidSignal == false) {
                return false; // channel did not have signal in any bin
            }
        }

        return true;
    }
};

/**
 * Evaluates if all the selected channels have frequency content in all the specified ranges, and none in the
 * rest of the spectrum.
 *
 * To count as 'there is frequency content', it needs to be above a certain threshold in dB (relative to the maximum bin).
 * If any other frequency ranges (FFT bins) reach this threshold, the result will be 'false'
 *
 * TODO: add option to re-use 'cache' the FFT results instead of recalculating every time.
 */
struct HasSignalOnlyInFrequencyRanges
{
    // TODO: add overload for single FrequencyRange
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, FrequencySelection frequencySelection,
                     float sampleRate, float threshold_dB = -0.5f)
    {
        // Determine bins where signal is expected
        std::set<int> expectedBins = FrequencyDomainHelpers::determineCorrespondingBins(frequencySelection, sampleRate);
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> normalizedBins = FrequencyDomainHelpers::getNormalizedFrequencyBins(channelSignal);
        
            bool hasValidSignal = false;
            for (int binIndex = 0; binIndex < FrequencyDomainHelpers::numBins; ++binIndex) {
                float binValue_dB = Utils::linear2Db(normalizedBins.at(binIndex));
                if (binValue_dB >= threshold_dB) {
                    // TODO: optimization -- change this to a vector<bool> or something (direct access rather than find)
                    if (expectedBins.find(binIndex) == expectedBins.end()) {
                        return false; // there's signal in at least one bin outside the selected range
                    }
                    hasValidSignal = true;
                }
            }
            if (hasValidSignal == false) {
                return false; // channel did not have signal in any bin
            }
        }
            
        return true;
    }
};


/** Can be used as a shorthand */
struct HasSignalOnlyBelow
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float frequency, float sampleRate, float threshold_dB = -0.5f)
    {
        return HasSignalOnlyInFrequencyRanges::eval(signal, selectedChannels, {FrequencyRange{1, frequency}}, sampleRate, threshold_dB);
    }
};

/** Can be used as a shorthand */
struct HasSignalOnlyAbove
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float frequency, float sampleRate, float threshold_dB = -0.5f)
    {
        return HasSignalOnlyInFrequencyRanges::eval(signal, selectedChannels, {FrequencyRange{frequency, sampleRate/2}}, sampleRate, threshold_dB);
    }
};

} // namespace AudioTraits
} // namespace slb
