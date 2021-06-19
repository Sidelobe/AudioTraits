//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"
#include "SignalGenerator.hpp"

#include <numeric>
#include <iostream>

#include "FFT/RealValuedFFT.hpp"

using namespace slb;
using namespace slb::AudioTraits;
using namespace TestCommon;
using namespace std::complex_literals;

std::vector<float> calculateNormalizedMagnitude(std::vector<std::complex<float>> binData)
{
    std::vector<float> magnitude;
    for (auto& binValue : binData) {
        magnitude.push_back(std::abs(binValue));
    }
    float maxValue = *std::max_element(magnitude.begin(), magnitude.end());
    maxValue = std::max(std::numeric_limits<float>::min(), maxValue); // avoid div by zero
    for (auto& e : magnitude) { e /= maxValue; }
    return magnitude;
}

double calculateStdDev(std::vector<float> data)
{
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    double mean = sum / data.size();

    double sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0);
    return std::sqrt(sq_sum / data.size() - mean * mean);
}

TEST_CASE("RealValuedFFT Tests")
{
    // choose some non-power-of-2 on purpose
    int fftLength = GENERATE(16, 500, 4000, 16384);
    
    RealValuedFFT fft(fftLength); // will choose next-higher power of 2
    const int N = Utils::nextPowerOfTwo(fftLength);
    const int numBins = N/2 + 1;
    int signalLength = N;
    
    SECTION("Forward Transform") {
    
        SECTION("Zeros / no signal") {
            std::vector<float> zeros(signalLength, 0);
            std::vector<std::complex<float>> result = fft.performForward(zeros);

            REQUIRE(result.size() == numBins);
            std::vector<std::complex<float>> expected(numBins, {0, 0});
            REQUIRE(std::equal(result.begin(), result.end(), expected.begin()));
            
            std::vector<float> magnitude = calculateNormalizedMagnitude(result);
            REQUIRE(magnitude == std::vector<float>(numBins, 0.f));
        }
        SECTION("Dirac Input") {
            std::vector<float> dirac = SignalGenerator::createDirac<float>(signalLength);
            std::vector<std::complex<float>> result = fft.performForward(dirac);
            
            REQUIRE(result.size() == numBins);
            std::vector<std::complex<float>> expected(numBins, {1, 0});
            REQUIRE(std::equal(result.begin(), result.end(), expected.begin(), [](auto& a, auto& b)
            {
                return std::abs(a-b) < 1e-6f;
            }));
            
            std::vector<float> magnitude = calculateNormalizedMagnitude(result);
            REQUIRE(magnitude == std::vector<float>(numBins, 1.f));
        }
        SECTION("Sine Input") {
            float sampleRate = 48e3f;
            float frequency = GENERATE(1000, 200, 10000, 24e3);
            std::vector<float> sine = SignalGenerator::createSine<float>(frequency, sampleRate, signalLength);
            std::vector<std::complex<float>> result = fft.performForward(sine);

            REQUIRE(result.size() == numBins);
            std::vector<float> magnitude = calculateNormalizedMagnitude(result);

            // the expected bin has an uncertainty of 1 (floor/ceil), so we always test expectedBin and expectedBin+1
            int expectedBin = static_cast<int>(std::floor(N * frequency / sampleRate));
            REQUIRE((magnitude[expectedBin] == 1.f || magnitude[expectedBin+1] == 1.f)); //normalization guarantees this

            // other bins are more than 4dB below
            for (int binIndex = 0; binIndex < numBins; ++binIndex) {
                if (binIndex == expectedBin) continue;
                if (N < 4096) {
                    REQUIRE((magnitude[binIndex] < Utils::dB2Linear(-4.f) || magnitude[binIndex+1] < Utils::dB2Linear(-4.f)));
                } else {
                    // for 4096 and above, we get 6 dB "SNR"
                    REQUIRE((magnitude[binIndex] < Utils::dB2Linear(-6.f) || magnitude[binIndex+1] < Utils::dB2Linear(-6.f)));
                }
            }
            if (N >= 512) {
                REQUIRE(calculateStdDev(magnitude) <= 0.0879f);
            }
        }
        SECTION("Noise Input") {
            std::vector<float> noise = SignalGenerator::createWhiteNoise(signalLength);
            std::vector<std::complex<float>> result = fft.performForward(noise);
 
            REQUIRE(result.size() == numBins);
            
            if (N >= 512) {
                std::vector<float> magnitude = calculateNormalizedMagnitude(result);
                REQUIRE(calculateStdDev(magnitude) <= 0.18f);
            }
        }
    }
    SECTION("Inverse Transform") {
        std::vector<std::complex<float>> zeros(numBins, {0, 0});
        std::vector<float> result = fft.performInverse(zeros);
        
        REQUIRE(result.size() == N);
        std::vector<float> expected(N, 0);
        REQUIRE(std::equal(result.begin(), result.end(), expected.begin()));
    }
    
    SECTION("Chain of FFT and IFFT") {        
        std::vector<float> noise = SignalGenerator::createWhiteNoise(signalLength);
        std::vector<std::complex<float>> result = fft.performForward(noise);

        std::vector<float> restoredNoise = fft.performInverse(result);
        REQUIRE(restoredNoise.size() == noise.size());
        REQUIRE(std::equal(noise.begin(), noise.end(), restoredNoise.begin(), [](auto& a, auto& b)
        {
            return std::abs(a-b) < 1e-6f;
        }));
    }

    SECTION("Numeric Example: Ramp Signal") {
        
        if (N == 16) {
            std::vector<float> testSequence { 0.062500, 0.125000, 0.187500, 0.250000, 0.312500, 0.375000, 0.437500, 0.500000,
                                              0.562500, 0.625000, 0.687500, 0.750000, 0.812500, 0.875000, 0.937500, 1.000000 };

            REQUIRE(testSequence.size() == N);
            
            std::vector<std::complex<float>> result = fft.performForward(testSequence);
        
            // Results from FFT in Matlab - (only from DC until Nyquist frequency)
            const std::vector<std::complex<float>> reference {
                 8.5000f + 0.0000if,
                -0.5000f + 2.5137if,
                -0.5000f + 1.2071if,
                -0.5000f + 0.7483if,
                -0.5000f + 0.5000if,
                -0.5000f + 0.3341if,
                -0.5000f + 0.2071if,
                -0.5000f + 0.0995if,
                -0.5000f + 0.0000if
            };
            REQUIRE(reference.size() == numBins);
            
            REQUIRE(std::equal(result.begin(), result.end(), reference.begin(), [](auto a, auto b)
            {
                return std::abs(a-b) < 5e-4f;
            }));
            
            std::vector<float> restoredSequence = fft.performInverse(result);
        
            REQUIRE(std::equal(restoredSequence.begin(), restoredSequence.end(), testSequence.begin(), [](auto a, auto b)
            {
                return std::abs(a-b) < 5e-4f;
            }));
        }
    }
}
