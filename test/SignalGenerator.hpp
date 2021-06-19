//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <vector>
#include <set>
#include <random>

#include "Utils.hpp"
#include "FrequencySelection.hpp"

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
    std::vector<T> noise = createWhiteNoise(length, gain_dB, seed);

    ASSERT(sampleRate > 0);
    const double Q = M_SQRT1_2;
    
    float coeffsHP[5];
    {
        double frequency = std::get<0>(band.get());
        double w0 = 2 * M_PI * frequency / sampleRate;
        double alpha  = std::sin(w0) / (2*Q);
        double cosine  = std::cos(w0);
        double a0 =  1.f + alpha;
        coeffsHP[0] = static_cast<float>((1.f + cosine) / 2.f / a0);
        coeffsHP[1] = static_cast<float>((-1.f - cosine) / a0);
        coeffsHP[2] = coeffsHP[0];
        coeffsHP[3] = static_cast<float>(-2.f * cosine / a0);
        coeffsHP[4] = static_cast<float>((1.f - alpha) / a0);
    }

    float coeffsLP[5];
    {
        double frequency = std::get<1>(band.get());
        double w0 = 2 * M_PI * frequency / sampleRate;
        double alpha  = std::sin(w0) / (2*Q);
        double cosine  = std::cos(w0);
        double a0 =  1.f + alpha;
        coeffsLP[0] = static_cast<float>((1.f - cosine) / 2.f / a0);
        coeffsLP[1] = static_cast<float>((1.f - cosine) / a0);
        coeffsLP[2] = coeffsLP[0];
        coeffsLP[3] = static_cast<float>((-2.f * cosine) / a0);
        coeffsLP[4] = static_cast<float>((1.f - alpha) / a0);
    }
    
    auto processBiquad = [](std::vector<T>& audio, float* coeffs, float* states)
    {
        float* inputs = audio.data();
        float* outputs = audio.data();
        for (int k=0; k < audio.size(); ++k) {
            float in = *inputs++;
            float out = coeffs[0] * in;
            out += coeffs[1] * states[0];
            out += coeffs[2] * states[1];
            out -= coeffs[3] * states[2];
            out -= coeffs[4] * states[3];
            states[1] = states[0];
            states[0] = in;
            states[3] = states[2];
            states[2] = out;
            *outputs++ = out;
         }
    };
    
    // every call applies the filter with 12 dB/octave
    
    float statesHP[4] {0.f};
    processBiquad(noise, coeffsHP, statesHP);
    processBiquad(noise, coeffsHP, statesHP);
    
    float statesLP[4] {0.f};
    processBiquad(noise, coeffsLP, statesLP);
    processBiquad(noise, coeffsLP, statesLP);

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

