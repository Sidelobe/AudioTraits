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
    
    std::vector<float> dirac = createDirac<float>(N);
    
    std::vector<std::complex<float>> result = fftForward.perform(dirac);
    
    std::vector<std::complex<float>> expected(N+1, {1, 0});
    REQUIRE(std::equal(result.begin(), result.end(), expected.begin(),
                       [](auto& a, auto& b) { return std::abs(a-b) < 1e-3f; })
            );

}
