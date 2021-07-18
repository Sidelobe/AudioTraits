//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <algorithm>
#include <set>
#include <vector>

#include "ChannelSelection.hpp"
#include "SignalAdapters.hpp"

#include "FFT/RealValuedFFT.hpp"
#include "FrequencySelection.hpp"

namespace slb {
namespace AudioTraits {

// MARK: - Constants
constexpr int fftLength = 4096;
static_assert(Utils::isPowerOfTwo(fftLength), "FFT has to be power of 2");
constexpr int numBins = fftLength / 2 + 1;

// MARK: - Helper functions
template<typename T=float>
inline void applyHannWindow(std::vector<T>& channelSignal)
{
    int i=0;
    for (auto& sample : channelSignal) {
        double window = 0.5 * (1 - std::cos(2*M_PI * i++ / (channelSignal.size()-1)));
        sample *= static_cast<T>(window);
    }
}

/** Create list of bins that correspond to FrequencySelection */
inline std::set<int> determineCorrespondingBins(const FrequencySelection& frequencySelection, float sampleRate)
{
    std::set<int> bins;
    
    for (auto& frequencyRange : frequencySelection.get()) {
        float freqStart = std::get<0>(frequencyRange);
        float freqEnd = std::get<1>(frequencyRange);
        int expectedBinStart = static_cast<int>(std::floor(freqStart / sampleRate * fftLength));
        int expectedBinEnd = static_cast<int>(std::ceil(freqEnd / sampleRate * fftLength));
        ASSERT(expectedBinStart >= 0, "invalid frequency range");
        ASSERT(expectedBinEnd < numBins, "frequency range too high for this sampling rate");
        for (int i=expectedBinStart; i <= expectedBinEnd; ++i) {
            bins.insert(i);
        }
    }
    return bins;
}

inline std::vector<float> getNormalizedFrequencyBins(std::vector<float>& channelSignal)
{
    // zero-pad to a minimum signal length, fixed FFT size
    // need windowing between the calls? (apply before zero-padding
    /*
         % Experiment in Octave
         signalLengthSeconds = 1;
         fs = 48e3;
         t = [0:1/fs:signalLengthSeconds-1/fs];
         signal = sin(2*pi*t * 1000); %+ 0.5*sin(2*pi*t * 10000);

         fftN = 16384;
         spectrum = fft(signal, fftN);
         spectrum = spectrum(1:fftN/2);
         mag = abs(spectrum ./fs); % scaling

         freqVec = (0:fftN/2-1)*fs/fftN;
         plot(freqVec, mag);
     */

    RealValuedFFT fft(fftLength);
    
    // perform FFT in several chunks
    constexpr int chunkSize = fftLength;
    float numChunksFract = static_cast<float>(channelSignal.size()) / chunkSize;
    int numChunks = static_cast<int>(std::ceil(numChunksFract));
    channelSignal.resize(numChunks * chunkSize); // pad to a multiple of full chunks
    
    // Accumulated over all chunks - init with 0
    std::vector<float> accumulatedBins(numBins, 0.f);
    
    for (int chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex) {
        auto chunkBegin = channelSignal.begin() + chunkIndex * chunkSize;
        std::vector<float> chunkTimeDomain{chunkBegin, chunkBegin + chunkSize};
        
        applyHannWindow(chunkTimeDomain);
        
        std::vector<std::complex<float>> freqDomainData = fft.performForward(chunkTimeDomain);
        std::vector<float> binValuesForChunk;
        for (auto& binValue : freqDomainData) {
            binValuesForChunk.emplace_back(std::abs(binValue));
        }
        
        // accumulate: accumulatedBins += binValues
        ASSERT(binValuesForChunk.size() == accumulatedBins.size());
        std::transform(accumulatedBins.begin(), accumulatedBins.end(), binValuesForChunk.begin(),
                       accumulatedBins.begin(), std::plus<float>());
    }
    
    // normally, we would normalize then bin values with numChunks and fftLength, but here
    // we choose to define the highest-valued bin as 0dB, therefore we normalize by it
    float maxBinValue = *std::max_element(accumulatedBins.begin(), accumulatedBins.end());
    for (auto& binValue : accumulatedBins) {
        binValue /= maxBinValue;
    }
    return accumulatedBins;
}

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
        std::set<int> expectedBins = determineCorrespondingBins(frequencySelection, sampleRate);
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> normalizedBins = getNormalizedFrequencyBins(channelSignal);
            
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
        std::set<int> expectedBins = determineCorrespondingBins(frequencySelection, sampleRate);
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> normalizedBins = getNormalizedFrequencyBins(channelSignal);
        
            bool hasValidSignal = false;
            for (int binIndex = 0; binIndex < numBins; ++binIndex) {
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
