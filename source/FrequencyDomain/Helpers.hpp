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

#include "FrequencyDomain/RealValuedFFT.hpp"
#include "FrequencySelection.hpp"

namespace slb {
namespace AudioTraits {

namespace FrequencyDomainHelpers
{
// MARK: - Constants
constexpr int fftLength = 4096;
static_assert(Utils::isPowerOfTwo(fftLength), "FFT has to be power of 2");
constexpr int numBins = fftLength / 2 + 1;

// MARK: - Helper functions
template<typename T=float>
inline void applyHannWindow(std::vector<T>& channelSignal)
{
    int i = 0;
    for (auto& sample : channelSignal) {
        double window = 0.5 * (1 - std::cos(2*M_PI * i++ / (channelSignal.size()-1)));
        sample *= static_cast<T>(window);
    }
}

/** Create list of bins that correspond to one FrequencyRange */
static inline std::set<int> determineCorrespondingBins(const FreqBand& frequencyRange, float sampleRate)
{
    std::set<int> bins;
    
    float freqStart = std::get<0>(frequencyRange.get());
    float freqEnd = std::get<1>(frequencyRange.get());
    int expectedBinStart = static_cast<int>(std::floor(freqStart / sampleRate * fftLength));
    int expectedBinEnd = static_cast<int>(std::ceil(freqEnd / sampleRate * fftLength));
    ASSERT(expectedBinStart >= 0, "invalid frequency range");
    ASSERT(expectedBinEnd < numBins, "frequency range too high for this sampling rate");
    
    for (int i=expectedBinStart; i <= expectedBinEnd; ++i) {
        bins.insert(i);
    }
    return bins;
}

/** Create an aggregated list of bins that correspond to all in bands in the selection */
static inline std::set<int> determineCorrespondingBins(const Freqs& frequencySelection, float sampleRate)
{
    std::set<int> bins;
    
    for (const auto& frequencyRange : frequencySelection.getRanges()) {
        std::set<int> binsForThisRange = determineCorrespondingBins(frequencyRange, sampleRate);
        bins.insert(binsForThisRange.begin(), binsForThisRange.end());
    }
    return bins;
}

/** @returns the absolute values of the bin contents for a given signal, normalized to the highest-valued bin */
static inline std::vector<float> getNormalizedBinValues(std::vector<float>& channelSignal)
{
    // TODO: use overlap-add for cleaner results (?)

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
        for (const auto& binValue : freqDomainData) {
            binValuesForChunk.emplace_back(std::abs(binValue));
        }
        
        // accumulate: accumulatedBins += binValues
        ASSERT(binValuesForChunk.size() == accumulatedBins.size());
        for (int k=0; k < accumulatedBins.size(); ++k) {
            accumulatedBins[k] += binValuesForChunk[k];
        }
    }
    
    // normally, we would normalize then bin values with numChunks and fftLength, but here
    // we choose to define the highest-valued bin as 0dB, therefore we normalize by it
    float maxBinValue = *std::max_element(accumulatedBins.begin(), accumulatedBins.end());
    for (auto& binValue : accumulatedBins) {
        binValue /= maxBinValue;
    }
    
    // Hard-code DC bin to 0
    accumulatedBins[0] = 0;

    return accumulatedBins;
}
} // namespace FrequencyDomainHelpers

} // namespace AudioTraits
} // namespace slb
