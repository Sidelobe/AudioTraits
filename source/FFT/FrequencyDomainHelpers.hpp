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

#include "FFT/RealValuedFFT.hpp"
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

/** @returns the absolute values of the bin contents for a given signal, normalized to the highest-valued bin */
inline std::vector<float> getNormalizedBinValues(std::vector<float>& channelSignal)
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
    
    return accumulatedBins;
}
} // namespace FrequencyDomainHelpers

} // namespace AudioTraits
} // namespace slb
