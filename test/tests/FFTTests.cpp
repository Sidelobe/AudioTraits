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
    RealValuedFFT fftForward(8);
    
    std::vector<float> dirac = createDirac<float>(8);
    
    std::vector<std::complex<float>> result = fftForward.perform(dirac);
    

}
