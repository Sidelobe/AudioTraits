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

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C"
{
#include "DSPF_sp_fftSPxSP_cn.h"
#include "DSPF_sp_ifftSPxSP_cn.h"
#include "TI_FFT_support.h"
}
#ifdef __clang__
    #pragma clang diagnostic pop
#elif defined __GNUC__
    #pragma GCC diagnostic pop
#endif


namespace slb {

class RealValuedFFT
{
public:
    explicit RealValuedFFT(int fftLength) :
        m_fftLength(Utils::nextPowerOfTwo(fftLength)),
        m_splitTableA(m_fftLength/2),
        m_splitTableB(m_fftLength/2),
        m_twiddleTable(m_fftLength/2)
    {
        const int N = m_fftLength / 2;
     
        if (N == 16 || N == 64 || N == 256 || N == 1024 || N == 4096) {
            m_radix = 4; // is power of 4 (exponent even?)
        } else if (N == 8 || N == 32 || N == 128 || N == 512 || N == 2048 || N == 8192) {
            m_radix = 2; // is power of 2 and not of 4 (exponent odd?)
        } else {
            ASSERT_ALWAYS("Length not supported");
        }

        tw_gen(reinterpret_cast<float*>(&m_twiddleTable[0]), N);
        split_gen(reinterpret_cast<float*>(&m_splitTableA[0]), reinterpret_cast<float*>(&m_splitTableB[0]), N);
    }
    
    /** Calculates the FFT for a real-valued input - using a split-complex FFT */
    std::vector<std::complex<float>> performForward(const std::vector<float>& realInput)
    {
        ASSERT(realInput.size() >= m_fftLength, "Signal length must match FFT Size"); // TODO: zero-padding
        
        // Trick: We calculate a complex FFT of length N/2  ('split complex FFT')
        const int N = m_fftLength / 2;
        
        // Split input sequence into a pseudo-complex signal (second half is imag part)
        std::vector<std::complex<float>> pseudoComplexInput(N);
        for (int k = 0; k < pseudoComplexInput.size(); ++k) {
            pseudoComplexInput.at(k) = { realInput.at(2*k+0), realInput.at(2*k+1) };
        }
        
        std::vector<std::complex<float>> complexOutput(N+1);
        const int offset = 0;

        // Forward FFT Calculation using a N-point complex FFT
        DSPF_sp_fftSPxSP(N, reinterpret_cast<float*>(pseudoComplexInput.data()),
                         reinterpret_cast<float*>(m_twiddleTable.data()),
                         reinterpret_cast<float*>(complexOutput.data()),
                         const_cast<unsigned char*>(brev_data),
                         m_radix, offset, N);

        std::vector<std::complex<float>> freqDomainBuffer(m_fftLength+1); // entire length +1 required for calculation
        FFT_Split(N, reinterpret_cast<float*>(complexOutput.data()),
                  reinterpret_cast<float*>(m_splitTableA.data()),
                  reinterpret_cast<float*>(m_splitTableB.data()),
                  reinterpret_cast<float*>(freqDomainBuffer.data()));

        // TODO: make buffers members so there's no allocation
        
        return {freqDomainBuffer.begin(), freqDomainBuffer.begin() + N+1}; // only return fftLength/2+1 complex pairs
    }
    
    std::vector<float> performInverse(const std::vector<std::complex<float>>& complexInput)
    {
        const int N = m_fftLength / 2;
        std::vector<std::complex<float>> tempComplexBuffer(complexInput.size());
        
        IFFT_Split(N, reinterpret_cast<const float*>(complexInput.data()),
                   reinterpret_cast<float*>(m_splitTableA.data()),
                   reinterpret_cast<float*>(m_splitTableB.data()),
                   reinterpret_cast<float*>(tempComplexBuffer.data()));
        
        std::vector<float> timeDomainBuffer(m_fftLength);
        const int offset = 0;
        
        // Inverse FFT Calculation using N/2 complex IFFT
        DSPF_sp_ifftSPxSP(N, reinterpret_cast<float*>(tempComplexBuffer.data()),
                          reinterpret_cast<float*>(m_twiddleTable.data()),
                          reinterpret_cast<float*>(timeDomainBuffer.data()),
                          const_cast<unsigned char*>(brev_data),
                          m_radix, offset, N);
        
        // TODO: make buffers members so there's no allocation
        return timeDomainBuffer;
    }
    
private:
    int m_fftLength;
    int m_radix;
    
    std::vector<std::complex<float>> m_splitTableA;
    std::vector<std::complex<float>> m_splitTableB;
    std::vector<std::complex<float>> m_twiddleTable;
};

} // namespace slb
