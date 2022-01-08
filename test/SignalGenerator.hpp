//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <complex>
#include <vector>
#include <set>
#include <random>

#include "Utils.hpp"
#include "FrequencySelection.hpp"
#include "FrequencyDomain/Helpers.hpp"

// TODO: consider moving to production code.

namespace slb {
namespace AudioTraits {
namespace SignalGenerator
{

using stl_size_type = typename std::vector<float>::size_type;
constexpr stl_size_type STL(int i) { return static_cast<stl_size_type>(i); }

template<typename T>
static std::vector<T> createSilence(int length)
{
    return std::vector<T>(length, 0);
}

/** @returns an std::vector with pseudo-random values between [-1, 1] */
template<typename T = float>
static std::vector<T> createWhiteNoise(int length, float gain_dB = 0.0, int seed=0)
{
    // NOTE: this pseudo-random number generation is not guaranteed to be identical on every
    // machine/compiler/stdlib, just identical every time it is called in a given environment.
    std::vector<T> result(STL(length));
    std::mt19937 engine(static_cast<unsigned>(seed));
    std::uniform_real_distribution<> dist(-1, 1); //(inclusive, inclusive)
    T gain = Utils::dB2Linear(gain_dB);
    for (auto& sample : result) {
        sample = gain * static_cast<T>(dist(engine));
    }
    return result;
}

/** Creates noise restricted to a certain frequency band. The filtering happens with a 4th-order filter (24dB/octave slope) */
template<typename T>
static std::vector<T> createBandLimitedNoise(int length, slb::FrequencyRange band, float sampleRate, float gain_dB = 0.0, int seed=0)
{
    // TODO: Implement this cleanly. overlapp-add ?
    ASSERT_ALWAYS("NOT YET IMPLEMENTED");
    
    std::vector<T> noise = createWhiteNoise(length, gain_dB, seed);

    ASSERT(sampleRate > 0);
        
    // check out:
    // https://s3.amazonaws.com/embeddedrelated/user/6420/sdr_narrowband_noise_4_69237.pdf
    {
        auto originalLength = noise.size();
        
        // FFT Filtering
        constexpr int fftLength = 4096;
        auto numBins = fftLength / 2 + 1;
        RealValuedFFT fft(fftLength);
        
        float freqStart = std::get<0>(band.get());
        float freqEnd = std::get<1>(band.get());
        int expectedBinStart = static_cast<int>(std::floor(freqStart / sampleRate * fftLength));
        int expectedBinEnd = static_cast<int>(std::ceil(freqEnd / sampleRate * fftLength));
    
        // perform FFT in several chunks
        constexpr int chunkSize = fftLength;
        float numChunksFract = static_cast<float>(noise.size()) / chunkSize;
        int numChunks = static_cast<int>(std::ceil(numChunksFract));
        noise.resize(numChunks * chunkSize); // pad to a multiple of full chunks

        std::vector<T> bandLimitedNoise(noise.size(), 0);
        
        for (int chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex) {
            auto chunkBegin = noise.begin() + chunkIndex * chunkSize;

            std::vector<T> chunkTimeDomain{chunkBegin, chunkBegin + chunkSize};
            FrequencyDomainHelpers::applyHannWindow(chunkTimeDomain);
            
            std::vector<std::complex<T>> bins = fft.performForward(chunkTimeDomain);
       
            // Zero bins that are out of range
            for (int binIndex = 0 ; binIndex < numBins; ++binIndex) {
                if (binIndex < expectedBinStart || binIndex > expectedBinEnd) {
                    bins[binIndex] = 0;
                } else {
                    // fix magnitude at 1.0, randomize phase: use time-domain signal (which is random already)
                    bins[binIndex] = std::exp(std::complex<T>{0, noise[chunkIndex*binIndex] });
                }
            }

            auto filteredChunkTD = fft.performInverse(bins);
            ASSERT(filteredChunkTD.size() == chunkSize);
            
            auto chunkBeginBandLimited = bandLimitedNoise.begin() + chunkIndex * chunkSize;
            std::copy(filteredChunkTD.begin(), filteredChunkTD.end(), chunkBeginBandLimited);
        }
        
        bandLimitedNoise.resize(originalLength);
    
        return bandLimitedNoise;
    }
    
    return noise;
}

static inline std::vector<int> createRandomVectorInt(int length, int seed=0)
{
    std::vector<int> result(STL(length));
    std::mt19937 engine(static_cast<unsigned>(seed));
    std::uniform_real_distribution<> dist(-1, 1); //(inclusive, inclusive)
    for (auto& sample : result) {
        sample = static_cast<int>(1000*dist(engine));
    }
    return result;
}

template<typename T>
static std::vector<T> createDirac(int lengthSamples)
{
    std::vector<T> result(lengthSamples);
    result[0] = static_cast<T>(1.0);
    std::fill(result.begin()+1, result.end(), static_cast<T>(0.0));
    return result;
}

template<typename T>
static std::vector<T> createSine(float frequency, float fs, int lengthSamples, float gain_dB = 0.f)
{
    std::vector<T> result(lengthSamples);
    double angularFrequency = 2 * M_PI * frequency / fs;
    double phase = angularFrequency;
    float gain = Utils::dB2Linear(gain_dB);
    for (auto& sample : result) {
        phase = std::fmod(phase + angularFrequency, 2 * M_PI);
        sample = static_cast<T>(gain * std::sin(phase));
    }
    return result;
}


} // namespace SignalGenerator
} // namespace AudioTraits
} // namespace slb

