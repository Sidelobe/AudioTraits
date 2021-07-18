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
        
    float coeffsHP[5]; // {b0, b1, b2, a1, a2}
    {
        constexpr double Q = M_SQRT1_2;
        double frequency = std::get<0>(band.get());
        double w0 = 2 * M_PI * frequency / sampleRate;
        double alpha  = std::sin(w0) / (2*Q);
        double cosine  = std::cos(w0);
        double a0 =  1.0 + alpha;
        coeffsHP[0] = static_cast<float>((1.f + cosine) / 2.f / a0);
        coeffsHP[1] = static_cast<float>((-1.f - cosine) / a0);
        coeffsHP[2] = coeffsHP[0];
        coeffsHP[3] = static_cast<float>(-2.f * cosine / a0);
        coeffsHP[4] = static_cast<float>((1.f - alpha) / a0);
    }

    float coeffsLP[5]; // {b0, b1, b2, a1, a2}
    {
        constexpr double Q = M_SQRT1_2;
        double frequency = std::get<1>(band.get());
        double w0 = 2 * M_PI * frequency / sampleRate;
        double alpha  = std::sin(w0) / (2*Q);
        double cosine  = std::cos(w0);
        double a0 =  1.0 + alpha;
        coeffsLP[0] = static_cast<float>((1.f - cosine) / 2.f / a0);
        coeffsLP[1] = static_cast<float>((1.f - cosine) / a0);
        coeffsLP[2] = coeffsLP[0];
        coeffsLP[3] = static_cast<float>((-2.f * cosine) / a0);
        coeffsLP[4] = static_cast<float>((1.f - alpha) / a0);
    }
//    float coeffsBP[5]; // {b0, b1, b2, a1, a2}
//    {
//        constexpr double Q = 5; // very narrow
//        double frequency = std::get<0>(band.get()) + 0.5 * (std::get<1>(band.get()) - std::get<0>(band.get())) ;
//        double w0 = 2 * M_PI * frequency / sampleRate;
//        double K = std::tan(w0 / 2);
//        double N = 1.0 / (K*K + K/Q + 1.0);
//        coeffsBP[0] = static_cast<float>(N * K/Q);
//        coeffsBP[1] = 0.f;
//        coeffsBP[2] = -coeffsBP[0];
//        coeffsBP[3] = static_cast<float>(2 * N * (K*K - 1.0));
//        coeffsBP[4] = static_cast<float>(N * (K*K - K/Q + 1.0));
//    }
    
    // Synthesize IIR Butterworth Bandpass Filters using biquads
    // https://www.dsprelated.com/showarticle/1257.php
    constexpr int N = 5; // number of biquads in BPF
    float coeffsBP[N][5]; // {b0, b1, b2, a1, a2}
    float gainsBP[N] { 1 };
    {
        float f1 = std::get<0>(band.get());
        float f2 = std::get<1>(band.get());
        
        // find poles of butterworth LPF with Wc = 1 rad/s
        for (int k=0; k < N; ++k) {
            double theta= (2*k+1) * M_PI/(2*N);
            std::complex<double> pole {-std::sin(theta), std::cos(theta)};
            
            // pre-warp f0, f1, and f2 (uppercase == continuous frequency variables)
            double F1 = sampleRate/M_PI * std::tan(M_PI*f1/sampleRate);
            double F2 = sampleRate/M_PI * std::tan(M_PI*f2/sampleRate);
            double bandWidth = F2 - F1;
            double F0 = std::sqrt(F1*F2); // geometric mean frequency (Hz)
            
            constexpr std::complex<double> one {1, 0};
            constexpr std::complex<double> i {0, 1};
            // transform poles for bpf centered at W0
            // pa contains N poles of the total 2N -- the other N poles not computed (are conjugates of these)
            std::complex<double> alpha = bandWidth/F0 * 0.5 * pole;
            std::complex<double> x = (0.5 * bandWidth/F0 * pole);
            std::complex<double> beta = std::sqrt(one - std::pow(x, 2));
            
            std::complex<double> pA = 2 * M_PI * F0 * (alpha + i * beta);
            
            // find poles of digital filter
            pA /= 2 * sampleRate; // normalize
            std::complex<double> p = (one + pA) / (one - pA);    // bilinear transform
            
            // biquad numerator coeffs
            coeffsBP[k][0] =  1; // b0
            coeffsBP[k][1] =  0; // b1
            coeffsBP[k][2] = -1; // b2
            
            // biquad denominator coeffs
            coeffsBP[k][3] = static_cast<float>(-2 * p.real()); // a1
            coeffsBP[k][4] = static_cast<float>(std::abs(std::pow(p, 2))); // a2
            
            double f0 = std::sqrt(f1*f2); // geometric mean frequency
            
            // TODO: need to calculate FFT magnitude here
            //double h = ; // freq response at f=f0
            //gainsBP[k] = 1.f / std::abs(h);
        }
    }
    
    auto processBiquad = [](std::vector<T>& audio, const float* const coeffs, float* states)
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
    
//    float statesHP[4] {0.f};
//    processBiquad(noise, coeffsHP, statesHP);
//    processBiquad(noise, coeffsHP, statesHP);
//    processBiquad(noise, coeffsHP, statesHP);
//    processBiquad(noise, coeffsHP, statesHP);
//
//    float statesLP[4] {0.f};
//    processBiquad(noise, coeffsLP, statesLP);
//    processBiquad(noise, coeffsLP, statesLP);
//    processBiquad(noise, coeffsLP, statesLP);
//    processBiquad(noise, coeffsLP, statesLP);
    
//    float statesBP[4] {0.f};
//    processBiquad(noise, coeffsBP, statesBP);
//    processBiquad(noise, coeffsBP, statesBP);
//    processBiquad(noise, coeffsBP, statesBP);
    
    auto summedNoisePaths = std::vector<float>(noise.size(), 0);
    for (int b=0; b < N; ++b) {
        float statesBP[4] {0.f};
        std::vector<float> noisePath = noise;
        processBiquad(noisePath, coeffsBP[b], statesBP);

        for (int k=0; k < noisePath.size(); ++k) {
            summedNoisePaths[k] += noisePath[k  ] * gainsBP[b];
        }
    }

    return summedNoisePaths;
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

