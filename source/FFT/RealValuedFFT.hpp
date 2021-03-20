//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <cmath>
#include <array>
#include <complex>
#include <vector>

#include "Utils.hpp"

extern "C"
{
#include "DSPF_sp_fftSPxSP_cn.h"
#include "DSPF_sp_ifftSPxSP_cn.h"
#include "TI_FFT_support.h"
}

namespace slb {

class RealValuedFFT
{
public:
    RealValuedFFT(int fftLength) :
        m_fftLength(fftLength),
        m_splitTableA(fftLength/2),
        m_splitTableB(fftLength/2),
        m_twiddleTable(fftLength/2)
    {
        // TODO: power of 2
        const int N = fftLength/2;
        tw_gen(reinterpret_cast<float*>(&m_twiddleTable[0]), N);
        split_gen(reinterpret_cast<float*>(&m_splitTableA[0]), reinterpret_cast<float*>(&m_splitTableB[0]), N);
        
        if (N == 16 || N == 64 || N == 256 || N == 1024 || N == 4096 || N == 16384) {
            m_radix = 4;
        } else if (N == 8 || N == 32 || N == 128 || N == 512 || N == 2048 || N == 8192) {
            m_radix = 2;
        } else {
            ASSERT_ALWAYS("Length not supported");
        }

        tw_gen(reinterpret_cast<float*>(&m_twiddleTable[0]), N);
        split_gen(reinterpret_cast<float*>(&m_splitTableA[0]), reinterpret_cast<float*>(&m_splitTableB[0]), N);
    }
    
    /** Calculates the FFT for a real-valued input - using a split-complex FFT */
    std::vector<std::complex<float>> performForward(const std::vector<float>& realInput)
    {
        ASSERT(realInput.size() == m_fftLength, "Signal length must match FFT Size");
        // TODO: zero-pad
        
        // Trick: We calculate a complex FFT of length N/2  ('split complex FFT')
        const int N = m_fftLength / 2;
        
        // Split input sequence into a pseudo-complex signal (second half is imag part)
        std::vector<std::complex<float>> pseudoComplexInput(N);
        for (int k = 0; k < pseudoComplexInput.size(); ++k) {
            pseudoComplexInput[k] = { realInput[2*k+0], realInput[2*k+1] };
        }
        
        std::vector<std::complex<float>> complexOutput(N+1);
        const int offset = 0;

        // Forward FFT Calculation using a N-point complex FFT
        // TODO: casts
        DSPF_sp_fftSPxSP(N, (float*)&pseudoComplexInput[0], (float*)&m_twiddleTable[0], (float*)&complexOutput[0],
                         (unsigned char*) brev.data(), m_radix, offset, N);

        std::vector<std::complex<float>> result(m_fftLength+1); // entire length +1 required for calculation 
        FFT_Split(N, (float*)&complexOutput[0], (float*)&m_splitTableA[0], (float*)&m_splitTableB[0], (float*)&result[0]);

        // TODO: make buffers members so there's no allocation
        
        return {result.begin(), result.begin() + N+1}; // only return fftLength/2+1 complex pairs
    }
    
    std::vector<float> performInverse(const std::vector<std::complex<float>>& complexInput)
    {
        const int N = m_fftLength/2;
        std::vector<std::complex<float>> tempComplexBuffer(N+1);
        
        // TODO: casts
        IFFT_Split(N, (float*)&complexInput[0], (float*)&m_splitTableA[0], (float*)&m_splitTableB[0], (float*)&tempComplexBuffer[0]);
        
        std::vector<float> result(m_fftLength);
        const int offset = 0;
        
        // Inverse FFT Calculation using N/2 complex IFFT
        DSPF_sp_ifftSPxSP(N, (float*)&tempComplexBuffer[0], (float*)&m_twiddleTable[0], (float*)&result[0],
                          (unsigned char*) brev.data(), m_radix, offset, N);
        
        // TODO: make buffers members so there's no allocation
        return result;
    }
    
private:
    int m_fftLength;
    int m_radix;
    
    std::vector<std::complex<float>> m_splitTableA;
    std::vector<std::complex<float>> m_splitTableB;
    std::vector<std::complex<float>> m_twiddleTable;
};

} // namespace slb
