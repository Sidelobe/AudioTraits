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
//#include "FFTReal.hpp"

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
        const int N = fftLength/2;
        tw_gen(reinterpret_cast<float*>(&m_twiddleTable[0]), N);
        split_gen(reinterpret_cast<float*>(&m_splitTableA[0]), reinterpret_cast<float*>(&m_splitTableB[0]), N);
        
        if (N == 4 || N == 16 || N == 64 || N == 256 || N == 1024 || N == 4096 || N == 16384) {
            m_radix = 4;
        } else if (N == 8 || N == 32 || N == 128 || N == 512 || N == 2048 || N == 8192) {
            m_radix = 2;
        } else {
            ASSERT_ALWAYS("Length not supported");
        }
    }
    
    /** Calculates the FFT for a real-valued input - using a split-complex FFT */
    std::vector<std::complex<float>> perform(const std::vector<float>& realInput)
    {
        ASSERT(realInput.size() == m_fftLength, "Signal length must match FFT Size");
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

        std::vector<std::complex<float>> result(m_fftLength+1);
        FFT_Split(N, (float*)&complexOutput[0], (float*)&m_splitTableA[0], (float*)&m_splitTableB[0], (float*)&result[0]);

        // TODO: make buffers members so there's no allocation
        
        return {result.begin(), result.begin() + N+1}; // only return fftLength/2+1 complex pairs
    }
    
private:
    /**
     * Split the result to obtain a 2*N =fftLength complex sequence
     *
     * From TI's spra291:
     * @code
     *  X(N) = X(0)
     *  for k=0 .. N-1
     *      Gr(k) = Xr(k) Ar(k) – Xi(k) Ai(k) + Xr(N–k) Br(k) + Xi(N–k) Bi(k)
     *      Gi(k) = Xi(k) Ar(k) + Xr(k) Ai(k) + Xr(N–k) Bi(k) – Xi(N–k) Br(k)
     *      Gr(2N–k) = Gr(k)
     *      Gi(2N–k) = –Gi(k)
     *  Gr(N) = Gr(0) – Gr(1)
     *  Gi(N) = 0
     * @endcode
     */
    std::vector<std::complex<float>> split(std::vector<std::complex<float>>& splittableSequence)
    {
        const int N = m_fftLength / 2;
        ASSERT(splittableSequence.size() == N, "Input has wrong length");
        std::vector<std::complex<float>> result(m_fftLength);
        
        // aliases
        decltype(splittableSequence)& X = splittableSequence;
        decltype(m_splitTableA)& A = m_splitTableA;
        decltype(m_splitTableB)& B = m_splitTableB;
        
        X[N] = X[0];
        for (int k = 0; k < N; ++k) {
            float real = X[k].real()   * A[k].real() - X[k].imag()   * A[k].imag()
                       + X[N-k].real() * B[k].real() + X[N-k].imag() * B[k].imag();
            
            float imag = X[k].imag()   * A[k].real() + X[k].real()   * A[k].imag()
                       + X[N-k].real() * B[k].imag() - X[N-k].imag() * B[k].real();
            
            result[k] = { real, imag };
            
            // Use complex conjugate symmetry properties for the second half
            result[2*k] = std::conj(result[k]);
        }
        result[N] = { result[0].real() - result[1].real(), 0.f };
        return result;
        
    }
    
private:
    int m_fftLength;
    
    std::vector<std::complex<float>> m_splitTableA;
    std::vector<std::complex<float>> m_splitTableB;
    std::vector<std::complex<float>> m_twiddleTable;
    
    int m_radix;

};

class RealValuedIFFT
{
public:
    RealValuedIFFT(int fftLength) {}
    
    void process(float* input, float** complex);
private:
    
};


} // namespace slb
