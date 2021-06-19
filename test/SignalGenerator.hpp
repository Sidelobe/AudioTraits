//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <vector>
#include <random>

#include "Utils.hpp"

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

