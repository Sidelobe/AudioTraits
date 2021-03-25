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

// for frequency-domain traits
#include "FFT/RealValuedFFT.hpp"
#include "FrequencySelection.hpp"

namespace slb {
namespace AudioTraits {

// MARK: - Infrastructure
template<typename F, typename ... Is>
static bool check(const ISignal& signal, const ChannelSelection& channelSelection, Is&& ... traitParams)
{
    ASSERT(signal.getNumSamples() > 0);
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

/** @returns true if a >= b (taking into account tolerance [dB]) */
static inline bool areVectorsEqual(const std::vector<float>& a, const std::vector<float>& b, float tolerance_dB)
{
    ASSERT(a.size() == b.size(), "Vectors must be of equal length for comparison");
    return std::equal(a.begin(), a.end(), b.begin(), [&tolerance_dB](float a, float b)
    {
        float error = std::abs(Utils::linear2Db(std::abs(a)) - Utils::linear2Db(std::abs(b)));
        return error <= tolerance_dB;
    });
};


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
        ASSERT(delay_samples >= 0, "The delay must be positive");
        ASSERT(amplitudeTolerance_dB >= 0 && amplitudeTolerance_dB < 96.f, "Invalid amplitude tolerance");
        ASSERT(timeTolerance_samples >= 0 && timeTolerance_samples <= 5, "Time tolerance has to be between 0 and 5 samples");
        ASSERT((static_cast<float>(delay_samples)/signal.getNumSamples()) < .8f, "The delay cannot be longer than 80% of the signal");
        ASSERT(referenceSignal.getNumSamples() >= signal.getNumSamples() - delay_samples, "The reference signal is not long enough");

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
        ASSERT(tolerance_dB >= 0 && tolerance_dB < 96.f, "Invalid amplitude tolerance");
        
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
        ASSERT(tolerance_dB >= 0 && tolerance_dB < 96.f, "Invalid amplitude tolerance");
        
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


// MARK: - Frequency-Domain Traits (TODO: move to separate file?)

/**
 * Evaluates if all the selected channels have frequency content in all the specified ranges, and none in the
 * rest of the spectrum. Frequency content is considered above a certain threshold in dB (relative to full scale)
 */
struct HasSignalOnlyInFrequencyRanges
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, FrequencySelection frequencySelection,
                     float sampleRate, float threshold_dB = -8.f, float narrowness_Hz = 10)
    {
        // TODO: how to analyze frequencies efficiently?
        // zero-pad to a minimum signal length, fixed FFT size
        // need windowing between the calls? (apply before zero-padding
        //
        
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
        const int fftLength = 4096;
        ASSERT(Utils::nextPowerOfTwo(fftLength) == fftLength, "FFT has to be power of 2");
        int numBins = fftLength / 2 + 1;
        RealValuedFFT fft(fftLength);
        
        // TODO: loop over all ranges -- or create list of 'acceptable' bins
        std::pair<float, float> firstSelectedRange = *frequencySelection.get().begin();
        float freqStart = std::get<0>(firstSelectedRange);
        float freqEnd = std::get<1>(firstSelectedRange);
        int expectedBinStart = static_cast<int>(std::floor(freqStart / sampleRate * fftLength));
        int expectedBinEnd = static_cast<int>(std::ceil(freqEnd / sampleRate * fftLength));
        ASSERT(expectedBinStart >= 0, "invalid frequency range");
        ASSERT(expectedBinEnd < numBins, "frequency range too high for this sampling rate");
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            
            bool hasValidSignal = false;
            
            // TODO: add windowing
            // perform FFT in several chunks
            int chunkSize = fftLength;
            float numChunksFract = static_cast<float>(channelSignal.size()) / chunkSize;
            int numChunks = static_cast<int>(std::ceil(numChunksFract));
            channelSignal.resize(numChunks * chunkSize); // pad to a multiple of full chunks
            
            // Average over chunks - init with 0
            std::vector<float> avgMagnitude(numBins, 0.f);
            
            for (int chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex) {
                auto chunkBegin = channelSignal.begin() + chunkIndex * chunkSize;
                auto chunkEnd = chunkBegin + chunkSize;

                std::vector<std::complex<float>> freqData = fft.performForward({chunkBegin, chunkEnd});
                std::vector<float> magnitude;
                for (auto& binValue : freqData) {
                    magnitude.push_back(std::abs(binValue) / fftLength); // scaling
                }
                // TODO: combine
                for (auto& binMag : magnitude) {
                    binMag /= numChunks; // averaging
                }
                
                // avgFreqData += freqData
                ASSERT(magnitude.size() == avgMagnitude.size());
                std::transform(avgMagnitude.begin(), avgMagnitude.end(), magnitude.begin(), avgMagnitude.begin(), std::plus<float>());
            }
            
            for (int binIndex = 0; binIndex < numBins; ++binIndex) {
                float mag_dB = Utils::linear2Db(avgMagnitude.at(binIndex));
                bool binHasContent = (mag_dB >= threshold_dB);
                
                if (binHasContent) {
                    if (binIndex < expectedBinStart || binIndex > expectedBinEnd) {
                        return false; // there's signal in at least one bin outside the selected range
                    } else {
                        hasValidSignal = true;
                    }
                }
            }
            if (hasValidSignal == false) {
                return false; // channel did not have signal in any bin
            }
        }
        return true;
    }
};

} // namespace AudioTraits
} // namespace slb
