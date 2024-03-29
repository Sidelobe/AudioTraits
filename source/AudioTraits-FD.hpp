//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//  
//  © 2023 Lorenz Bucher - all rights reserved
//  https://github.com/Sidelobe/AudioTraits

#pragma once

// NOTE: DO NOT INCLUDE THIS FILE DIRECTLY; INCLUDE THIS INSTEAD: AudioTraits.hpp

#include <algorithm>
#include <vector>

#include "ChannelSelection.hpp"
#include "SignalAdapters.hpp"

#include "FrequencySelection.hpp"
#include "FrequencyDomain/Helpers.hpp"

namespace slb {
namespace AudioTraits {

// MARK: - Frequency Domain Audio Traits

/**
 * Evaluates if all the selected channels have frequency content in all the specified bands.
 * The spectral content outside the specified bands is not analyzed.
 *
 * To count as 'there is frequency content', it needs to be above a certain threshold in dB in at least one of the bins
 * in that band. The threshold is relative to the maximum bin value of all bins, across the entire spectrum.
 *
 * TODO: add option to re-use 'cache' the FFT results instead of recalculating every time.
 */
struct HasSignalInAllBands
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, const Freqs& frequencySelection,
                     float sampleRate, float threshold_dB = -0.5f)
    {
        if (frequencySelection.getRanges().empty()) {
            return false; // Empty frequency selection is always false
        }
        
        // Each frequency band needs to be tested individually
        for (const auto& frequencyRange : frequencySelection.getRanges()) {
            // Determine bins where signal is expected
            std::set<int> expectedBins = FrequencyDomainHelpers::determineCorrespondingBins(frequencyRange, sampleRate);
            
            for (int chNumber : selectedChannels) {
                std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
                std::vector<float> normalizedBinValues = FrequencyDomainHelpers::getNormalizedBinValues(channelSignal);
                
                bool hasValidSignalInThisRange = false;
                for (int expectedBin : expectedBins) {
                    float binValue_dB = Utils::linear2Db(normalizedBinValues.at(expectedBin));
                    if (binValue_dB >= threshold_dB) {
                        hasValidSignalInThisRange = true;
                        break; // at least one of the bins in this band has signal, skip the remaining bins in this band.
                    }
                }
                if (hasValidSignalInThisRange == false) {
                    return false; // channel did not have signal in any bin in this band
                }
            }
        }
        
        return true;
    }
};

/**
 * Evaluates if all the selected channels have frequency content in all the specified bands only, and none in the
 * rest of the spectrum. Note that the signal *can* have content in the specified bands, but does not necessarily *have
 * to* for this trait to be true.
 *
 * To count as 'there is frequency content', it needs to be above a certain threshold in dB in at least one of the bins
 * in that band. The threshold is relative to the maximum bin value of all bins, across the entire spectrum.
 *
 * If any FFT bins (that are not part of the selected frequency bands) reach the threshold, the result will be 'false'.
 *
 * TODO: add option to re-use 'cache' the FFT results instead of recalculating every time.
 */
struct HasSignalOnlyInBands
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, const Freqs& frequencySelection,
                     float sampleRate, float threshold_dB = -0.5f)
    {
        // We only need to scan 'illegal' bands for content. If these are clean, the trait is true.
        // Determine bins where signal is allowed
        std::set<int> legalBins = FrequencyDomainHelpers::determineCorrespondingBins(frequencySelection, sampleRate);
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> normalizedBinValues = FrequencyDomainHelpers::getNormalizedBinValues(channelSignal);
        
            for (int binIndex = 0; binIndex < FrequencyDomainHelpers::numBins; ++binIndex) {
                float binValue_dB = Utils::linear2Db(normalizedBinValues.at(binIndex));
                if (binValue_dB >= threshold_dB) {
                    // there's content in this bin -- is this bin 'legal' ?
                    // TODO: optimization -- change this to a vector<bool> or something (direct access rather than find)
                    if (legalBins.find(binIndex) == legalBins.end()) {
                        // this bin is not legal -> there's signal in at least one bin outside the legal bands
                        return false;
                    }
                }
            }
        }
            
        return true;
    }
};


/** Can be used as a shorthand for HasSignalOnlyInBands, where the lower limit of the band is the minimum frequency (1Hz)*/
struct HasSignalOnlyBelow
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float frequency, float sampleRate, float threshold_dB = -0.5f)
    {
        return HasSignalOnlyInBands::eval(signal, selectedChannels, Freqs{{1, frequency}}, sampleRate, threshold_dB);
    }
};

/** Can be used as a shorthand for HasSignalOnlyInBands, where the upper limit of the band is the maximum frequency (Nyquist=samplerate/2) */
struct HasSignalOnlyAbove
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float frequency, float sampleRate, float threshold_dB = -0.5f)
    {
        return HasSignalOnlyInBands::eval(signal, selectedChannels, Freqs{{frequency, sampleRate/2}}, sampleRate, threshold_dB);
    }
};

} // namespace AudioTraits
} // namespace slb
