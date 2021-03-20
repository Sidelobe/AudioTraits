//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#include "TestCommon.hpp"

#include "FFT/RealValuedFFT.hpp"

using namespace slb;
using namespace TestCommon;

TEST_CASE("FFT Tests")
{
    int N = GENERATE(8, 16, 64, 1024);
    RealValuedFFT fftForward(N);
    
    auto compareApprox = [](auto& a, auto& b)
    {
        return std::abs(a-b) < 1e-6f;
    };
    
    auto compareEnergy = [](auto& a, auto& b)
    {
        return std::abs(std::abs(a) - std::abs(b)) < 1e-6f;
    };
    
    SECTION("Dirac Input") {
        std::vector<float> dirac = createDirac<float>(N);
        std::vector<std::complex<float>> result = fftForward.perform(dirac);
        
        std::vector<std::complex<float>> expected(N/2+1, {1, 0});
        REQUIRE(std::equal(result.begin(), result.end(), expected.begin(), compareApprox));
    }
    SECTION("Noise Input") {
        std::vector<float> noise = createRandomVector(N);
        std::vector<std::complex<float>> result = fftForward.perform(noise);

        std::vector<std::complex<float>> expected(N/2+1, {1, 0});
        REQUIRE(std::equal(result.begin(), result.end(), expected.begin(), compareApprox));
    }
    SECTION("Pure Sine Input") {
        float frequency = GENERATE(1000, 200, 10000);
        std::vector<float> sine = createSine<float>(frequency, 48e3, N);
        std::vector<std::complex<float>> result = fftForward.perform(sine);

        std::vector<float> mag(result.size()/2+1);
        for (int k = 0; k < mag.size(); ++k) {
            mag[k] = Utils::linear2Db(std::abs(result[k]));
        }
        std::vector<std::complex<float>> expected(N/2+1, {1, 0});
        //REQUIRE(std::equal(result.begin(), result.end(), expected.begin(), compareEnergy));
    }
}
