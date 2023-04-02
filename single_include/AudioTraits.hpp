// NOTE: This file is an amalgamation of individual source files to create a single-header include

//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once


#include <algorithm>
#include <array>
#include <cassert>
#include <complex>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <set>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

extern "C" 
{
#include <math.h>
}

// ADD SOME CHANGES HERE TO TEST GITHUB ACTION

// MARK: -------- Utils.hpp --------
//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved



#define UNUSED(x) (void)x

/* Macro to detect if exceptions are disabled (works on GCC, Clang and MSVC) 3 */
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#if !__has_feature(cxx_exceptions) && !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
  #define SLB_EXCEPTIONS_DISABLED
#endif

// GCC
//  Bug 67371 - Never executed "throw" in constexpr function fails to compile
//  https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86678
//  Fixed for GCC 9.
#if defined(__GNUC__) && (__GNUC__ < 9)
#   define BUG_67371
#endif

namespace slb
{

// MARK: - Assertion handling
namespace Assertions
{
#ifndef SLB_ASSERT
    #define SLB_ASSERT(condition, ...) Assertions::handleAssert(#condition, condition, __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef SLB_ASSERT_ALWAYS
    #define SLB_ASSERT_ALWAYS(...) Assertions::handleAssert("", false, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/**
 * Custom assertion handler
 *
 * @note: this assertion handler is constexpr - to allow its use inside constexpr functions.
 * The handler will still be evaluated at runtime, but memory is only allocated IF the assertion is triggered.
 */
#ifdef BUG_67371
static void handleAssert(const char* conditionAsText, bool condition, const char* file, int line, const char* message = "")
#else
static constexpr void handleAssert(const char* conditionAsText, bool condition, const char* file, int line, const char* message = "")
#endif
{
    if (condition == true) {
        return;
    }
    
#ifdef SLB_EXCEPTIONS_DISABLED
    UNUSED(conditionAsText); UNUSED(file); UNUSED(line); UNUSED(message);
    assert(0 && message);
#else
    throw std::runtime_error(std::string("Assertion failed: ") + conditionAsText + " (" + file + ":" + std::to_string(line) + ") " + message);
#endif
}
} // namespace Assertions


namespace Utils
{

static inline float dB2Linear(float value_dB)
{
    return std::pow(10.f, (value_dB/20.f));
}

static inline float linear2Db(float value_linear)
{
    // avoid log(0)
    if (value_linear > 0.f) {
        return 20.f * std::log10(value_linear);
    }
    return std::numeric_limits<float>::lowest();
}

/**
 * @note from: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 */
static inline uint32_t nextPowerOfTwo(uint32_t i)
{
    i--;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i++;
    return i;
}

constexpr bool isPowerOfTwo(uint32_t v)
{
    return v && ((v & (v - 1)) == 0);
}

} // namespace Utils


} // namespace slb

// MARK: -------- ChannelSelection.hpp --------
//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved




namespace slb
{

class SelectionItem
{
public:
    /** Discrete (deliberately non-explicit ctor) */
    SelectionItem(int channel) : m_selection({channel})
    {
        SLB_ASSERT(channel > 0, "invalid channel!");
    }
    
    /**
     * Range of channels (deliberately non-explicit ctor)
     * TODO: consider making this explicit -- maybe rename class to something shorter in this case
     */
    SelectionItem(int first, int last) : m_selection(last - first + 1)
    {
        SLB_ASSERT(first > 0 && last >= first, "invalid range!");
        std::iota(m_selection.begin(), m_selection.end(), first);
    }
    
    std::set<int> get() const { return { m_selection.begin(), m_selection.end() }; }
    int size() const { return static_cast<int>(m_selection.size()); }

private:
    std::vector<int> m_selection;
};


class ChannelSelection
{
public:
    ChannelSelection(std::initializer_list<SelectionItem> selectionItems) : m_selectionItems(selectionItems) {}
    
    std::set<int> get() const
    {
        std::set<int> result;
        for (auto& item : m_selectionItems) {
            std::set<int> itemSelection = item.get();
            result.insert(itemSelection.begin(), itemSelection.end());
        }
        return result;
    }
    
private:
    std::vector<SelectionItem> m_selectionItems;

};


} // namespace slb

// MARK: -------- FrequencySelection.hpp --------
//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved




namespace slb
{

class FreqBand
{
public:
    using Bounds = std::pair<float, float>;
    
    /**
     * Discrete frequency component (deliberately non-explicit ctor).
     * Treated as a range with equal upper and lower bound.
     */
    FreqBand(float frequencyComponent) : m_range{std::make_pair(frequencyComponent, frequencyComponent)}
    {
        SLB_ASSERT(frequencyComponent > 0 , "invalid frequency component");
    }
    
    /**
     * Range of frequencies (deliberately non-explicit ctor) 
     * TODO: consider making this explicit -- maybe rename class to something shorter in this case
     */
    FreqBand(float lowerBound, float uppperBound) : m_range{std::make_pair(lowerBound, uppperBound)}
    {
        SLB_ASSERT(uppperBound > lowerBound, "invalid range!");
        SLB_ASSERT(lowerBound > 0, "invalid lower bound!");
        SLB_ASSERT(uppperBound > 0, "invalid upper bound!");
    }
    
    Bounds get() const { return m_range; }
    float size() const { return std::abs(std::get<1>(m_range) - std::get<0>(m_range)); }
    float centerFrequency() const { return std::get<0>(m_range) + size()/2; }

private:
    Bounds m_range;
};


class Freqs
{
public:
    explicit Freqs(std::initializer_list<FreqBand> selectedRanges) : m_selectedRanges(selectedRanges) {}
    
    /** @return a duplicate-free list of FrequencyRange Bound pairs contained in the selection */
    std::set<FreqBand::Bounds> getBounds() const
    {
        std::set<FreqBand::Bounds> result;
        for (auto& item : m_selectedRanges) {
            result.insert(item.get());
        }
        return result;
    }
    
    /** @return a copy of all the FrequencyRanges in the selection */
    std::vector<FreqBand> getRanges() const
    {
        return m_selectedRanges;
    }
    
private:
    std::vector<FreqBand> m_selectedRanges;

};


} // namespace slb

// MARK: -------- SignalAdapters.hpp --------
//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved




namespace slb {
namespace AudioTraits {

// MARK: - Infrastructure

/**
 * Signal Interface - wraps around an existing signal of arbitrary type.
 *
 * Guarantees: will not modify the underlying signal, only analyze it
 */
class ISignal
{
public:
    virtual ~ISignal() = default;
    virtual int getNumChannels() const = 0;
    virtual int getNumSamples() const = 0;
    
    /** @returns a non-modifiable reference to the multichannel data */
    virtual const float* const* getData() const = 0;
    
    /** @returns a copy of the data of the given channelIndex (0-based) */
    virtual std::vector<float> getChannelDataCopy(int channelIndex) const = 0;
};

/**
 * Adapts a signal with raw pointers (float**) to the Signal Interface
 */
class SignalAdapterRaw : public ISignal
{
public:
    explicit SignalAdapterRaw(const float* const* rawSignal, int numChannels, int numSamples) :
        m_numChannels(numChannels),
        m_numSamples(numSamples),
        m_signal(rawSignal) {}

    int getNumChannels() const override { return m_numChannels; }
    int getNumSamples()  const override { return m_numSamples; }
    const float* const* getData() const override { return m_signal; }
    std::vector<float> getChannelDataCopy(int channelIndex) const override
    {
        SLB_ASSERT(channelIndex < m_numChannels);
        return { m_signal[channelIndex], m_signal[channelIndex] + m_numSamples };
    }
    
private:
    const int m_numChannels;
    const int m_numSamples;
    const float* const* m_signal;
};

/**
 * Adapts a std::vector<std::vector<float>> signal to the Signal Interface
 */
class SignalAdapterStdVecVec : public ISignal
{
public:
    explicit SignalAdapterStdVecVec(std::vector<std::vector<float>>& vector2D) :
        m_vector2D(vector2D),
        m_channelPointers(vector2D.size())
    {
        
        for (int i = 0; i < static_cast<int>(vector2D.size()); ++i) {
            SLB_ASSERT(m_vector2D.at(i).size() == m_vector2D.at(0).size(), "All channels should be of equal length!");
            m_channelPointers[i] = m_vector2D[i].data();
        }
    }
    
    int getNumChannels() const override { return static_cast<int>(m_vector2D.size()); }
    int getNumSamples()  const override { return static_cast<int>(m_vector2D.at(0).size()); }
    const float* const* getData() const override { return m_channelPointers.data(); }
    std::vector<float> getChannelDataCopy(int channelIndex) const override
    {
        return m_vector2D.at(channelIndex);
    }

private:
    const std::vector<std::vector<float>>& m_vector2D;
    std::vector<const float*> m_channelPointers;
};

} // namespace AudioTraits
} // namespace slb

// MARK: -------- FrequencyDomain/DSPF_sp_fftSPxSP_cn.h --------
extern "C" 
{

#ifndef _DSPF_SP_fftSPxSP_CN_H_
#define _DSPF_SP_fftSPxSP_CN_H_

/* ======================================================================= */
/*                                                                         */
/*   TEXAS INSTRUMENTS, INC.                                               */
/*                                                                         */
/*   NAME                                                                  */
/*    DSPF_sp_fftSPxSP - Single precision floating point FFT with          */
/*    complex input                                                        */
/*                                                                         */
/*   USAGE                                                                 */
/*         This routine is C-callable and can be called as:                */
/*                                                                         */
/*         void DSPF_sp_fftSPxSP(                                          */
/*             int N, float * ptr_x, float * ptr_w, float * ptr_y,         */
/*             unsigned char * brev, int n_min, int offset, int n_max);    */
/*                                                                         */
/*         N = length of fft in complex samples, power of 2 such that N>=8 */
/*             and N <= 16384.                                             */
/*         ptr_x = pointer to complex data input                           */
/*         ptr_w = pointer to complex twiddle factor (see below)           */
/*         brev = pointer to bit reverse table containing 64 entries       */
/*         n_min = smallest fft butterfly used in computation              */
/*                 used for decomposing fft into subffts, see notes        */
/*         offset = index in complex samples of sub-fft from start of main */
/*                  fft                                                    */
/*         n_max = size of main fft in complex samples                     */
/*                                                                         */
/*         (See the C compiler reference guide.)                           */
/*                                                                         */
/*   DESCRIPTION                                                           */
/*        The benchmark performs a mixed radix forwards fft using          */
/*        a special sequece of coefficients generated in the following     */
/*        way:                                                             */
/*                                                                         */
/*        void tw_gen(float * w, int N)                                    */
/*         {                                                               */
/*           int j, k;                                                     */
/*           double x_t, y_t, theta1, theta2, theta3;                      */
/*           const double PI = 3.141592654;                                */
/*                                                                         */
/*           for (j=1, k=0; j <= N>>2; j = j<<2)                           */
/*           {                                                             */
/*               for (i=0; i < N>>2; i+=j)                                 */
/*               {                                                         */
/*                   theta1 = 2*PI*i/N;                                    */
/*                   x_t = cos(theta1);                                    */
/*                   y_t = sin(theta1);                                    */
/*                   w[k]   =  (float)x_t;                                 */
/*                   w[k+1] =  (float)y_t;                                 */
/*                                                                         */
/*                   theta2 = 4*PI*i/N;                                    */
/*                   x_t = cos(theta2);                                    */
/*                   y_t = sin(theta2);                                    */
/*                   w[k+2] =  (float)x_t;                                 */
/*                   w[k+3] =  (float)y_t;                                 */
/*                                                                         */
/*                   theta3 = 6*PI*i/N;                                    */
/*                   x_t = cos(theta3);                                    */
/*                   y_t = sin(theta3);                                    */
/*                   w[k+4] =  (float)x_t;                                 */
/*                   w[k+5] =  (float)y_t;                                 */
/*                   k+=6;                                                 */
/*               }                                                         */
/*           }                                                             */
/*         }                                                               */
/*        This redundent set of twiddle factors is size 2*N float samples. */
/*        The function is accurate to about 130dB of signal to noise ratio */
/*        to the DFT function below:                                       */
/*                                                                         */
/*         void dft(int N, float x[], float y[])                           */
/*         {                                                               */
/*            int k,i, index;                                              */
/*            const float PI = 3.14159654;                                 */
/*            float * p_x;                                                 */
/*            float arg, fx_0, fx_1, fy_0, fy_1, co, si;                   */
/*                                                                         */
/*            for(k = 0; k<N; k++)                                         */
/*            {                                                            */
/*              p_x = x;                                                   */
/*              fy_0 = 0;                                                  */
/*              fy_1 = 0;                                                  */
/*              for(i=0; i<N; i++)                                         */
/*              {                                                          */
/*                fx_0 = p_x[0];                                           */
/*                fx_1 = p_x[1];                                           */
/*                p_x += 2;                                                */
/*                index = (i*k) % N;                                       */
/*                arg = 2*PI*index/N;                                      */
/*                co = cos(arg);                                           */
/*                si = -sin(arg);                                          */
/*                fy_0 += ((fx_0 * co) - (fx_1 * si));                     */
/*                fy_1 += ((fx_1 * co) + (fx_0 * si));                     */
/*              }                                                          */
/*              y[2*k] = fy_0;                                             */
/*              y[2*k+1] = fy_1;                                           */
/*            }                                                            */
/*         }                                                               */
/*                                                                         */
/*     The function takes the table and input data and calculates the fft  */
/*     producing the frequency domain data in the Y array.                 */
/*     As the fft allows every input point to effect every output point in */
/*     a cache based system such as the c6711, this causes cache thrashing.*/
/*     This is mitigated by allowing the main fft of size N to be divided  */
/*     into several steps, allowing as much data reuse as possible.        */
/*                                                                         */
/*     For example the following function:                                 */
/*                                                                         */
/*     DSPF_sp_fftSPxSP(1024, &x[0],&w[0],y,brev,4,  0,1024);              */
/*                                                                         */
/*     is equvalent to:                                                    */
/*                                                                         */
/*     DSPF_sp_fftSPxSP(1024,&x[2*0],  &w[0] ,   y,brev,256,  0,1024);     */
/*     DSPF_sp_fftSPxSP(256, &x[2*0],  &w[2*768],y,brev,4,    0,1024);     */
/*     DSPF_sp_fftSPxSP(256, &x[2*256],&w[2*768],y,brev,4,  256,1024);     */
/*     DSPF_sp_fftSPxSP(256, &x[2*512],&w[2*768],y,brev,4,  512,1024);     */
/*     DSPF_sp_fftSPxSP(256, &x[2*768],&w[2*768],y,brev,4,  768,1024);     */
/*                                                                         */
/*     Notice how the 1st fft function is called on the entire 1K data set */
/*     it covers the 1st pass of the fft until the butterfly size is 256.  */
/*     The following 4 ffts do 256 pt ffts 25% of the size. These continue */
/*     down to the end when the buttefly is of size 4. They use an index to*/
/*     the main twiddle factor array of 0.75*2*N. This is because the      */
/*     twiddle factor array is composed of successively decimated versions */
/*     of the main array.                                                  */
/*                                                                         */
/*     N not equal to a power of 4 can be used, i.e. 512. In this case to  */
/*     decompose the fft the following would be needed :                   */
/*                                                                         */
/*     DSPF_sp_fftSPxSP(512, &x[0],&w[0],y,brev,2,  0,512);                */
/*                                                                         */
/*     is equvalent to:                                                    */
/*                                                                         */
/*     DSPF_sp_fftSPxSP(512, &x[2*0],  &w[0] ,   y,brev,128,  0,512);      */
/*     DSPF_sp_fftSPxSP(128, &x[2*0],  &w[2*384],y,brev,4,    0,512);      */
/*     DSPF_sp_fftSPxSP(128, &x[2*128],&w[2*384],y,brev,4,  128,512);      */
/*     DSPF_sp_fftSPxSP(128, &x[2*256],&w[2*384],y,brev,4,  256,512);      */
/*     DSPF_sp_fftSPxSP(128, &x[2*384],&w[2*384],y,brev,4,  384,512);      */
/*                                                                         */
/*     The twiddle factor array is composed of log4(N) sets of twiddle     */
/*     factors, (3/4)*N, (3/16)*N, (3/64)*N, etc.  The index into this     */
/*     array for each stage of the fft is calculated by summing these      */
/*     indices up appropriately.                                           */
/*     For multiple ffts they can share the same table by calling the small*/
/*     ffts from further down in the twiddle factor array. In the same way */
/*     as the decomposition works for more data reuse.                     */
/*                                                                         */
/*     Thus, the above decomposition can be summarized for a general N,    */
/*     radix "rad" as follows:                                             */
/*     DSPF_sp_fftSPxSP(N,  &x[0],      &w[0],      y,brev,N/4,0,    N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[0],      &w[2*3*N/4],y,brev,rad,0,    N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[2*N/4],  &w[2*3*N/4],y,brev,rad,N/4,  N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[2*N/2],  &w[2*3*N/4],y,brev,rad,N/2,  N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[2*3*N/4],&w[2*3*N/4],y,brev,rad,3*N/4,N)    */
/*                                                                         */
/*     As discussed previously, N can be either a power of 4 or 2.         */
/*     If N is a power of 4, then rad = 4, and if N is a power of 2 and    */
/*     not a power of 4, then rad = 2. "rad" is used to control how many   */
/*     stages of decomposition are performed. It is also used to determine */
/*     whether a radix-4 or radix-2 decomposition should be performed at   */
/*     the last stage. Hence when "rad" is set to "N/4" the first stage of */
/*     the transform alone is performed and the code exits. To complete the*/
/*     FFT, four other calls are required to perform N/4 size FFTs.In fact,*/
/*     the ordering of these 4 FFTs amongst themselves does not matter and */
/*     hence from a cache perspective, it helps to go through the remaining*/
/*     4 FFTs in exactly the opposite order to the first.                  */
/*     This is illustrated as follows:                                     */
/*                                                                         */
/*     DSPF_sp_fftSPxSP(N,  &x[0],      &w[0],      y,brev,N/4,0,    N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[2*3*N/4],&w[2*3*N/4],y,brev,rad,3*N/4,N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[2*N/2],  &w[2*3*N/4],y,brev,rad,N/2,  N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[2*N/4],  &w[2*3*N/4],y,brev,rad,N/4,  N)    */
/*     DSPF_sp_fftSPxSP(N/4,&x[0],      &w[2*3*N/4],y,brev,rad,0,    N)    */
/*        In addition this function can be used to minimize call overhead, */
/*        by completing the FFT with one function call invocation as shown */
/*        below:                                                           */
/*     DSPF_sp_fftSPxSP(N,  &x[0],      &w[0],      y, brev, rad, 0, N)    */
/* ----------------------------------------------------------------------- */
/*      Copyright (c) 2003 Texas Instruments, Incorporated.                */
/*                     All Rights Reserved.                                */
/* ======================================================================= */

static void DSPF_sp_fftSPxSP(int n, float ptr_x[], float ptr_w[], float ptr_y[],
                             unsigned char brev[], int n_min, int offset, int n_max)
{
            int  i, j, k, l1, l2, h2, predj;
            int  tw_offset, stride, fft_jmp;
                                                                              
            float x0, x1, x2, x3,x4,x5,x6,x7;
            float xt0, yt0, xt1, yt1, xt2, yt2, yt3;
            float yt4, yt5, yt6, yt7;
            float si1,si2,si3,co1,co2,co3;
            float xh0,xh1,xh20,xh21,xl0,xl1,xl20,xl21;
            float x_0, x_1, x_l1, x_l1p1, x_h2 , x_h2p1, x_l2, x_l2p1;
            float xl0_0, xl1_0, xl0_1, xl1_1;
            float xh0_0, xh1_0, xh0_1, xh1_1;
            float *x,*w;
            int   k0, k1, j0, j1, l0, radix;
            float * y0, * ptr_x0, * ptr_x2;
                                                                              
            radix = n_min;
                                                                              
            stride = n; /* n is the number of complex samples*/
            tw_offset = 0;
            while (stride > radix)
            {
                j = 0;
                fft_jmp = stride + (stride>>1);
                h2 = stride>>1;
                l1 = stride;
                l2 = stride + (stride>>1);
                x = ptr_x;
                w = ptr_w + tw_offset;
                                                                              
                for (i = 0; i < n; i += 4)
                {
                    co1 = w[j];
                    si1 = w[j+1];
                    co2 = w[j+2];
                    si2 = w[j+3];
                    co3 = w[j+4];
                    si3 = w[j+5];
                                                                              
                    x_0    = x[0];
                    x_1    = x[1];
                    x_h2   = x[h2];
                    x_h2p1 = x[h2+1];
                    x_l1   = x[l1];
                    x_l1p1 = x[l1+1];
                    x_l2   = x[l2];
                    x_l2p1 = x[l2+1];
                                                                              
                    xh0  = x_0    + x_l1;
                    xh1  = x_1    + x_l1p1;
                    xl0  = x_0    - x_l1;
                    xl1  = x_1    - x_l1p1;
                                                                              
                    xh20 = x_h2   + x_l2;
                    xh21 = x_h2p1 + x_l2p1;
                    xl20 = x_h2   - x_l2;
                    xl21 = x_h2p1 - x_l2p1;
                                                                              
                    ptr_x0 = x;
                    ptr_x0[0] = xh0 + xh20;
                    ptr_x0[1] = xh1 + xh21;
                                                                              
                    ptr_x2 = ptr_x0;
                    x += 2;
                    j += 6;
                    predj = (j - fft_jmp);
                    if (!predj) x += fft_jmp;
                    if (!predj) j = 0;
                                                                              
                    xt0 = xh0 - xh20;
                    yt0 = xh1 - xh21;
                    xt1 = xl0 + xl21;
                    yt2 = xl1 + xl20;
                    xt2 = xl0 - xl21;
                    yt1 = xl1 - xl20;
                                                                              
                    ptr_x2[l1  ] = xt1 * co1 + yt1 * si1;
                    ptr_x2[l1+1] = yt1 * co1 - xt1 * si1;
                    ptr_x2[h2  ] = xt0 * co2 + yt0 * si2;
                    ptr_x2[h2+1] = yt0 * co2 - xt0 * si2;
                    ptr_x2[l2  ] = xt2 * co3 + yt2 * si3;
                    ptr_x2[l2+1] = yt2 * co3 - xt2 * si3;
                }
                tw_offset += fft_jmp;
                stride = stride>>2;
            }/* end while*/
                                                                              
            j = offset>>2;
                                                                              
            ptr_x0 = ptr_x;
            y0 = ptr_y;
            /*l0 = _norm(n_max) - 17;    get size of fft */
            l0=0;
            for(k=30;k>=0;k--)
                if( (n_max & (1 << k)) == 0 )
                   l0++;
                else
                   break;
            l0=l0-17;
            if (radix <= 4) for (i = 0; i < n; i += 4)
            {
                    /* reversal computation*/
                                                                              
                    j0 = (j     ) & 0x3F;
                    j1 = (j >> 6);
                    k0 = brev[j0];
                    k1 = brev[j1];
                    k = (k0 << 6) +  k1;
                    k = k >> l0;
                    j++;        /* multiple of 4 index*/
                                                                              
                    x0   = ptr_x0[0];  x1 = ptr_x0[1];
                    x2   = ptr_x0[2];  x3 = ptr_x0[3];
                    x4   = ptr_x0[4];  x5 = ptr_x0[5];
                    x6   = ptr_x0[6];  x7 = ptr_x0[7];
                    ptr_x0 += 8;
                                                                              
                    xh0_0  = x0 + x4;
                    xh1_0  = x1 + x5;
                    xh0_1  = x2 + x6;
                    xh1_1  = x3 + x7;
                                                                              
                    if (radix == 2) {
                      xh0_0 = x0;
                      xh1_0 = x1;
                      xh0_1 = x2;
                      xh1_1 = x3;
                    }
                                                                              
                    yt0  = xh0_0 + xh0_1;
                    yt1  = xh1_0 + xh1_1;
                    yt4  = xh0_0 - xh0_1;
                    yt5  = xh1_0 - xh1_1;
                                                                              
                    xl0_0  = x0 - x4;
                    xl1_0  = x1 - x5;
                    xl0_1  = x2 - x6;
                    xl1_1  = x3 - x7;
                                                                              
                    if (radix == 2) {
                      xl0_0 = x4;
                      xl1_0 = x5;
                      xl1_1 = x6;
                      xl0_1 = x7;
                    }
                                                                              
                    yt2  = xl0_0 + xl1_1;
                    yt3  = xl1_0 - xl0_1;
                    yt6  = xl0_0 - xl1_1;
                    yt7  = xl1_0 + xl0_1;
                                                                              
                    if (radix == 2) {
                      yt7  = xl1_0 - xl0_1;
                      yt3  = xl1_0 + xl0_1;
                    }
                                                                              
                    y0[k] = yt0; y0[k+1] = yt1;
                    k += n_max>>1;
                    y0[k] = yt2; y0[k+1] = yt3;
                    k += n_max>>1;
                    y0[k] = yt4; y0[k+1] = yt5;
                    k += n_max>>1;
                    y0[k] = yt6; y0[k+1] = yt7;
            }
}

/* ========================================================================*/
/*  End of file:  sp_fftSPxSP.c                                            */
/* ------------------------------------------------------------------------*/
/*            Copyright (c) 2003 Texas Instruments, Incorporated.          */
/*                           All Rights Reserved.                          */
/* ========================================================================*/

#endif // _DSPF_SP_fftSPxSP_CN_H_
} // extern "C"

// MARK: -------- FrequencyDomain/DSPF_sp_ifftSPxSP_cn.h --------
extern "C" 
{
#ifndef _DSPF_SP_ifftSPxSP_CN_H_
#define _DSPF_SP_ifftSPxSP_CN_H_

/* ========================================================================*/
/*  TEXAS INSTRUMENTS, INC.                                                */
/*                                                                         */
/*  NAME                                                                   */
/*      DSPF_sp_ifftSPxSP -- Single Precision floating point mixed radix   */
/*      inverse FFT with complex input                                     */
/*                                                                         */
/*  USAGE                                                                  */
/*          This routine is C-callable and can be called as:               */
/*                                                                         */
/*          void DSPF_sp_ifftSPxSP(                                        */
/*              int N, float * ptr_x, float * ptr_w, float * ptr_y,        */
/*              unsigned char * brev, int n_min, int offset, int n_max);   */
/*                                                                         */
/*          N = length of ifft in complex samples, power of 2 such that    */
/*              N>=8 and N <= 16384.                                       */
/*          ptr_x = pointer to complex data input (normal order)           */
/*          ptr_w = pointer to complex twiddle factor (see below)          */
/*          ptr_y = pointer to complex output data (normal order)          */
/*          brev = pointer to bit reverse table containing 64 entries      */
/*          n_min = smallest ifft butterfly used in computation            */
/*                  used for decomposing ifft into subiffts, see notes     */
/*          offset = index in complex samples of sub-ifft from start of    */
/*                   main ifft                                             */
/*          n_max = size of main ifft in complex samples                   */
/*                                                                         */
/*  DESCRIPTION                                                            */
/*         The benchmark performs a mixed radix forwards ifft using        */
/*         a special sequece of coefficients generated in the following    */
/*         way:                                                            */
/*                                                                         */
/*         //generate vector of twiddle factors for optimized algorithm//  */
/*          void tw_gen(float * w, int N)                                  */
/*          {                                                              */
/*            int j, k;                                                    */
/*            double x_t, y_t, theta1, theta2, theta3;                     */
/*            const double PI = 3.141592654;                               */
/*                                                                         */
/*            for (j=1, k=0; j <= N>>2; j = j<<2)                          */
/*            {                                                            */
/*                for (i=0; i < N>>2; i+=j)                                */
/*                {                                                        */
/*                    theta1 = 2*PI*i/N;                                   */
/*                    x_t = cos(theta1);                                   */
/*                    y_t = sin(theta1);                                   */
/*                    w[k]   =  (float)x_t;                                */
/*                    w[k+1] =  (float)y_t;                                */
/*                                                                         */
/*                    theta2 = 4*PI*i/N;                                   */
/*                    x_t = cos(theta2);                                   */
/*                    y_t = sin(theta2);                                   */
/*                    w[k+2] =  (float)x_t;                                */
/*                    w[k+3] =  (float)y_t;                                */
/*                                                                         */
/*                    theta3 = 6*PI*i/N;                                   */
/*                    x_t = cos(theta3);                                   */
/*                    y_t = sin(theta3);                                   */
/*                    w[k+4] =  (float)x_t;                                */
/*                    w[k+5] =  (float)y_t;                                */
/*                    k+=6;                                                */
/*                }                                                        */
/*            }                                                            */
/*          }                                                              */
/*        This redundant set of twiddle factors is size 2*N float samples. */
/*        The function is accurate to about 130dB of signal to noise ratio */
/*        to the IDFT function below:                                      */
/*                                                                         */
/*          void idft(int n, float x[], float y[])                         */
/*          {                                                              */
/*            int k,i, index;                                              */
/*            const float PI = 3.14159654;                                 */
/*            float * p_x;                                                 */
/*            float arg, fx_0, fx_1, fy_0, fy_1, co, si;                   */
/*                                                                         */
/*            for(k = 0; k<n; k++)                                         */
/*            {                                                            */
/*              p_x = x;                                                   */
/*              fy_0 = 0;                                                  */
/*              fy_1 = 0;                                                  */
/*              for(i=0; i<n; i++)                                         */
/*              {                                                          */
/*                fx_0 = p_x[0];                                           */
/*                fx_1 = p_x[1];                                           */
/*                p_x += 2;                                                */
/*                index = (i*k) % n;                                       */
/*                arg = 2*PI*index/n;                                      */
/*                co = cos(arg);                                           */
/*                si = sin(arg);                                           */
/*                fy_0 += ((fx_0 * co) - (fx_1 * si));                     */
/*                fy_1 += ((fx_1 * co) + (fx_0 * si));                     */
/*              }                                                          */
/*              y[2*k] = fy_0/n;                                           */
/*              y[2*k+1] = fy_1/n;                                         */
/*            }                                                            */
/*         }                                                               */
/*                                                                         */
/*         The function takes the table and input data and calculates the  */
/*         ifft producing the frequency domain data in the Y array. the    */
/*         output is scaled by a scaling factor of 1/N.                    */
/*                                                                         */
/*         As the ifft allows every input point to effect every output     */
/*         point in a cache based system such as the c6711, this causes    */
/*         cache thrashing. This is mitigated by allowing the main ifft    */
/*         of size N to be divided into several steps, allowing as much    */
/*         data reuse as possible.                                         */
/*                                                                         */
/*         For example the following function:                             */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(1024, &x[0],&w[0],y,brev,4,  0,1024)          */
/*                                                                         */
/*         is equvalent to:                                                */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(1024,&x[2*0],&w[0],y,brev,256,0,1024)         */
/*         DSPF_sp_ifftSPxSP(256,&x[2*0],&w[2*768],y,brev,4,0,1024)        */
/*         DSPF_sp_ifftSPxSP(256,&x[2*256],&w[2*768],y,brev,4,256,1024)    */
/*         DSPF_sp_ifftSPxSP(256,&x[2*512],&w[2*768],y,brev,4,512,1024)    */
/*         DSPF_sp_ifftSPxSP(256,&x[2*768],&w[2*768],y,brev,4,768,1024)    */
/*                                                                         */
/*         Notice how the 1st ifft function is called on the entire 1K     */
/*         data set it covers the 1st pass of the ifft until the butterfly */
/*         size is 256. The following 4 iffts do 256 pt iffts 25% of the   */
/*         size. These continue down to the end when the buttefly is of    */
/*         size 4. They use an index to the main twiddle factor array of   */
/*         0.75*2*N. This is because the twiddle factor array is composed  */
/*         of successively decimated versions of the main array.           */
/*                                                                         */
/*         N not equal to a power of 4 can be used, i.e. 512. In this case */
/*         to decompose the ifft the following would be needed :           */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(512, &x[0],&w[0],y,brev,2,  0,512)            */
/*                                                                         */
/*         is equvalent to:                                                */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(512, &x[2*0],  &w[0] ,   y,brev,128,  0,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*0],  &w[2*384],y,brev,4,    0,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*128],&w[2*384],y,brev,4,  128,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*256],&w[2*384],y,brev,4,  256,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*384],&w[2*384],y,brev,4,  384,512)  */
/*                                                                         */
/*         The twiddle factor array is composed of log4(N) sets of twiddle */
/*         factors, (3/4)*N, (3/16)*N, (3/64)*N, etc.  The index into this */
/*         array for each stage of the ifft is calculated by summing these */
/*         indices up appropriately.                                       */
/*         For multiple iffts they can share the same table by calling the */
/*         small iffts from further down in the twiddle factor array. In   */
/*         the same way as the decomposition works for more data reuse.    */
/*                                                                         */
/*         Thus, the above decomposition can be summarized for a general N */
/*         radix "rad" as follows:                                         */
/*         DSPF_sp_ifftSPxSP(N,  &x[0],      &w[0],      y,brev,N/4,0,   N)*/
/*         DSPF_sp_ifftSPxSP(N/4,&x[0],      &w[2*3*N/4],y,brev,rad,0,   N)*/
/*         DSPF_sp_ifftSPxSP(N/4,&x[2*N/4],  &w[2*3*N/4],y,brev,rad,N/4, N)*/
/*         DSPF_sp_ifftSPxSP(N/4,&x[2*N/2],  &w[2*3*N/4],y,brev,rad,N/2, N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*3*N/4],&w[2*3*N/4],y,brev,rad,3*N/4,N)*/
/*                                                                         */
/*         As discussed previously, N can be either a power of 4 or 2.     */
/*         If N is a power of 4, then rad = 4, and if N is a power of 2    */
/*         and not a power of 4, then rad = 2. "rad" is used to control    */
/*         how many stages of decomposition are performed. It is also      */
/*         used to determine whether a radix-4 or radix-2 decomposition    */
/*         should be performed at the last stage. Hence when "rad" is set  */
/*         to "N/4" the first stage of the transform alone is performed    */
/*         and the code exits. To complete the FFT, four other calls are   */
/*         required to perform N/4 size FFTs.In fact, the ordering of      */
/*         these 4 FFTs amongst themselves does not matter and hence from  */
/*         a cache perspective, it helps to go through the remaining 4     */
/*         FFTs in exactly the opposite order to the first. This is        */
/*         illustrated as follows:                                         */
/*                                                                         */
/*        DSPF_sp_ifftSPxSP(N,  &x[0],      &w[0],      y,brev,N/4,0,    N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*3*N/4],&w[2*3*N/4],y,brev,rad,3*N/4,N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*N/2],  &w[2*3*N/4],y,brev,rad,N/2,  N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*N/4],  &w[2*3*N/4],y,brev,rad,N/4,  N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[0],      &w[2*3*N/4],y,brev,rad,0,    N)*/
/*         In addition this function can be used to minimize call overhead,*/
/*         by completing the FFT with one function call invocation as      */
/*         shown below:                                                    */
/*        DSPF_sp_ifftSPxSP(N,  &x[0],      &w[0],      y, brev, rad, 0,N) */
/*                                                                         */
/* ------------------------------------------------------------------------*/
/*            Copyright (c) 2002 Texas Instruments, Incorporated.          */
/*                           All Rights Reserved.                          */
/* ========================================================================*/

static void DSPF_sp_ifftSPxSP(int N, float * ptr_x, float * ptr_w, float * ptr_y,
                              unsigned char * brev, int n_min, int offset, int n_max)
{
    int  i, j, k, l1, l2, h2, predj;
    int  tw_offset, stride, fft_jmp;
    
    float x0, x1, x2, x3,x4,x5,x6,x7;
    float xt0, yt0, xt1, yt1, xt2, yt2, yt3;
    float yt4, yt5, yt6, yt7;
    float si1,si2,si3,co1,co2,co3;
    float xh0,xh1,xh20,xh21,xl0,xl1,xl20,xl21;
    float x_0, x_1, x_l1, x_l1p1, x_h2 , x_h2p1, x_l2, x_l2p1;
    float xl0_0, xl1_0, xl0_1, xl1_1;
    float xh0_0, xh1_0, xh0_1, xh1_1;
    float *x,*w;
    int   k0, k1, j0, j1, l0, radix;
    float * y0, * ptr_x0, * ptr_x2;
    float scale;
    
    radix = n_min;
    
    stride = N; /* n is the number of complex samples*/
    tw_offset = 0;
    while (stride > radix)
    {
        j = 0;
        fft_jmp = stride + (stride>>1);
        h2 = stride>>1;
        l1 = stride;
        l2 = stride + (stride>>1);
        x = ptr_x;
        w = ptr_w + tw_offset;
        
        for (i = 0; i < N; i += 4)
        {
            co1 = w[j];
            si1 = w[j+1];
            co2 = w[j+2];
            si2 = w[j+3];
            co3 = w[j+4];
            si3 = w[j+5];
            
            x_0    = x[0];
            x_1    = x[1];
            x_h2   = x[h2];
            x_h2p1 = x[h2+1];
            x_l1   = x[l1];
            x_l1p1 = x[l1+1];
            x_l2   = x[l2];
            x_l2p1 = x[l2+1];
            
            xh0  = x_0    + x_l1;
            xh1  = x_1    + x_l1p1;
            xl0  = x_0    - x_l1;
            xl1  = x_1    - x_l1p1;
            
            xh20 = x_h2   + x_l2;
            xh21 = x_h2p1 + x_l2p1;
            xl20 = x_h2   - x_l2;
            xl21 = x_h2p1 - x_l2p1;
            
            ptr_x0 = x;
            ptr_x0[0] = xh0 + xh20;
            ptr_x0[1] = xh1 + xh21;
            
            ptr_x2 = ptr_x0;
            x += 2;
            j += 6;
            predj = (j - fft_jmp);
            if (!predj) x += fft_jmp;
            if (!predj) j = 0;
            
            xt0 = xh0 - xh20; //xt0 = xh0 - xh20;
            yt0 = xh1 - xh21; //yt0 = xh1 - xh21;
            xt1 = xl0 - xl21; //xt1 = xl0 + xl21;
            yt2 = xl1 - xl20; //yt2 = xl1 + xl20;
            xt2 = xl0 + xl21; //xt2 = xl0 - xl21;
            yt1 = xl1 + xl20; //yt1 = xl1 - xl20;
            
            ptr_x2[l1  ] = xt1 * co1 - yt1 * si1; //ptr_x2[l1  ] = xt1 * co1 + yt1 * si1;
            ptr_x2[l1+1] = yt1 * co1 + xt1 * si1; //ptr_x2[l1+1] = yt1 * co1 - xt1 * si1;
            ptr_x2[h2  ] = xt0 * co2 - yt0 * si2; //ptr_x2[h2  ] = xt0 * co2 + yt0 * si2;
            ptr_x2[h2+1] = yt0 * co2 + xt0 * si2; //ptr_x2[h2+1] = yt0 * co2 - xt0 * si2;
            ptr_x2[l2  ] = xt2 * co3 - yt2 * si3; //ptr_x2[l2  ] = xt2 * co3 + yt2 * si3;
            ptr_x2[l2+1] = yt2 * co3 + xt2 * si3; //ptr_x2[l2+1] = yt2 * co3 - xt2 * si3;
        }
        tw_offset += fft_jmp;
        stride = stride>>2;
    }/* end while*/
    
    j = offset>>2;
    
    ptr_x0 = ptr_x;
    y0 = ptr_y;
    /*l0 = _norm(n_max) - 17;    get size of fft */
    l0=0;
    for(k=30;k>=0;k--)
        if( (n_max & (1 << k)) == 0 )
            l0++;
        else
            break;
    l0=l0-17;
    scale = 1/(float)n_max;
    if (radix <= 4) for (i = 0; i < N; i += 4)
    {
        /* reversal computation*/
        
        j0 = (j     ) & 0x3F;
        j1 = (j >> 6);
        k0 = brev[j0];
        k1 = brev[j1];
        k = (k0 << 6) +  k1;
        k = k >> l0;
        j++;        /* multiple of 4 index*/
        
        x0   = ptr_x0[0];  x1 = ptr_x0[1];
        x2   = ptr_x0[2];  x3 = ptr_x0[3];
        x4   = ptr_x0[4];  x5 = ptr_x0[5];
        x6   = ptr_x0[6];  x7 = ptr_x0[7];
        ptr_x0 += 8;
        
        xh0_0  = x0 + x4;
        xh1_0  = x1 + x5;
        xh0_1  = x2 + x6;
        xh1_1  = x3 + x7;
        
        if (radix == 2) {
            xh0_0 = x0;
            xh1_0 = x1;
            xh0_1 = x2;
            xh1_1 = x3;
        }
        
        yt0  = xh0_0 + xh0_1;
        yt1  = xh1_0 + xh1_1;
        yt4  = xh0_0 - xh0_1;
        yt5  = xh1_0 - xh1_1;
        
        xl0_0  = x0 - x4;
        xl1_0  = x1 - x5;
        xl0_1  = x2 - x6;
        xl1_1  = x7 - x3; //xl1_1  = x3 - x7;
        
        if (radix == 2) {
            xl0_0 = x4;
            xl1_0 = x5;
            xl1_1 = x6;
            xl0_1 = x7;
        }
        
        yt2  = xl0_0 + xl1_1;
        yt3  = xl1_0 + xl0_1; // yt3  = xl1_0 + (- xl0_1);
        yt6  = xl0_0 - xl1_1;
        yt7  = xl1_0 - xl0_1; // yt7  = xl1_0 - (- xl0_1);
        
        y0[k] = yt0*scale; y0[k+1] = yt1*scale;
        k += n_max>>1;
        y0[k] = yt2*scale; y0[k+1] = yt3*scale;
        k += n_max>>1;
        y0[k] = yt4*scale; y0[k+1] = yt5*scale;
        k += n_max>>1;
        y0[k] = yt6*scale; y0[k+1] = yt7*scale;
    }
}

/* ========================================================================*/
/*  End of file: sp_ifftSPxSP.c                                            */
/* ------------------------------------------------------------------------*/
/*          Copyright (C) 2002 Texas Instruments, Incorporated.            */
/*                          All Rights Reserved.                           */
/* ========================================================================*/

#endif // _DSPF_SP_ifftSPxSP_CN_H_
} // extern "C"

// MARK: -------- FrequencyDomain/TI_FFT_support.h --------
extern "C" 
{
// From TI's FFT Demo/Benchmark project
// only modifications: commenting out platform specific pragmas and silencing double->float warnings

#ifndef TI_FFT_support_h
#define TI_FFT_support_h


#define PI M_PI

/** Bit-Reverse Table */
static const unsigned char brev_data[64] {
    0x0, 0x20, 0x10, 0x30, 0x8, 0x28, 0x18, 0x38,
    0x4, 0x24, 0x14, 0x34, 0xc, 0x2c, 0x1c, 0x3c,
    0x2, 0x22, 0x12, 0x32, 0xa, 0x2a, 0x1a, 0x3a,
    0x6, 0x26, 0x16, 0x36, 0xe, 0x2e, 0x1e, 0x3e,
    0x1, 0x21, 0x11, 0x31, 0x9, 0x29, 0x19, 0x39,
    0x5, 0x25, 0x15, 0x35, 0xd, 0x2d, 0x1d, 0x3d,
    0x3, 0x23, 0x13, 0x33, 0xb, 0x2b, 0x1b, 0x3b,
    0x7, 0x27, 0x17, 0x37, 0xf, 0x2f, 0x1f, 0x3f
};

/** Function for generating Specialized sequence of twiddle factors */
static void tw_gen (float *w, int n)
{
    int i, j, k;
    double x_t, y_t, theta1, theta2, theta3;

    for (j = 1, k = 0; j <= n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            theta1 = 2 * PI * i / n;
            x_t = cos (theta1);
            y_t = sin (theta1);
            w[k] = (float) x_t;
            w[k + 1] = (float) y_t;

            theta2 = 4 * PI * i / n;
            x_t = cos (theta2);
            y_t = sin (theta2);
            w[k + 2] = (float) x_t;
            w[k + 3] = (float) y_t;

            theta3 = 6 * PI * i / n;
            x_t = cos (theta3);
            y_t = sin (theta3);
            w[k + 4] = (float) x_t;
            w[k + 5] = (float) y_t;
            k += 6;
        }
    }
}

static void split_gen (float *pATable, float *pBTable, int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        pATable[2 * i] = (float) (0.5 * (1.0 - sin (2 * PI / (double) (2 * n) * (double) i)));
        pATable[2 * i + 1] = (float) (0.5 * (-1.0 * cos (2 * PI / (double) (2 * n) * (double) i)));
        pBTable[2 * i] = (float) (0.5 * (1.0 + sin (2 * PI / (double) (2 * n) * (double) i)));
        pBTable[2 * i + 1] = (float) (0.5 * (1.0 * cos (2 * PI / (double) (2 * n) * (double) i)));
    }
}

static void FFT_Split (int n, float *pIn, float *pATable, float *pBTable, float *pOut)
{
    int i;
    float Tr, Ti;

//    _nassert ((int) pIn % 8 == 0);
//    _nassert ((int) pOut % 8 == 0);
//    _nassert ((int) pATable % 8 == 0);
//    _nassert ((int) pBTable % 8 == 0);

    pIn[2 * n] = pIn[0];
    pIn[2 * n + 1] = pIn[1];

//#pragma UNROLL(2)
    for (i = 0; i < n; i++)
    {
        Tr = (pIn[2 * i] * pATable[2 * i] - pIn[2 * i + 1] * pATable[2 * i + 1] + pIn[2 * n - 2 * i] * pBTable[2 * i] + pIn[2 * n - 2 * i + 1] * pBTable[2 * i + 1]);

        Ti = (pIn[2 * i + 1] * pATable[2 * i] + pIn[2 * i] * pATable[2 * i + 1] + pIn[2 * n - 2 * i] * pBTable[2 * i + 1] - pIn[2 * n - 2 * i + 1] * pBTable[2 * i]);

        pOut[2 * i] = Tr;
        pOut[2 * i + 1] = Ti;
        // Use complex conjugate symmetry properties to get the rest..
        pOut[4 * n - 2 * i] = Tr;
        pOut[4 * n - 2 * i + 1] = -Ti;

    }
    pOut[2 * n] = pIn[0] - pIn[1];
    pOut[2 * n + 1] = 0;

}

static void IFFT_Split (int n, const float *pIn, float *pATable, float *pBTable, float *pOut)
{
    int i;
    float Tr, Ti;

//    _nassert ((int) pIn % 8 == 0);
//    _nassert ((int) pOut % 8 == 0);
//    _nassert ((int) pATable % 8 == 0);
//    _nassert ((int) pBTable % 8 == 0);

//#pragma UNROLL(2)
    for (i = 0; i < n; i++)
    {
        Tr = (pIn[2 * i] * pATable[2 * i] + pIn[2 * i + 1] * pATable[2 * i + 1] + pIn[2 * n - 2 * i] * pBTable[2 * i] - pIn[2 * n - 2 * i + 1] * pBTable[2 * i + 1]);

        Ti = (pIn[2 * i + 1] * pATable[2 * i] - pIn[2 * i] * pATable[2 * i + 1] - pIn[2 * n - 2 * i] * pBTable[2 * i + 1] - pIn[2 * n - 2 * i + 1] * pBTable[2 * i]);

        pOut[2 * i] = Tr;
        pOut[2 * i + 1] = Ti;
    }

}

#endif /* TI_FFT_support_h */
} // extern "C"

// MARK: -------- FrequencyDomain/RealValuedFFT.hpp --------
//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved




#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C"
{
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
            SLB_ASSERT_ALWAYS("Length not supported");
        }

        tw_gen(reinterpret_cast<float*>(&m_twiddleTable[0]), N);
        split_gen(reinterpret_cast<float*>(&m_splitTableA[0]), reinterpret_cast<float*>(&m_splitTableB[0]), N);
    }
    
    /** Calculates the FFT for a real-valued input - using a split-complex FFT */
    std::vector<std::complex<float>> performForward(const std::vector<float>& realInput)
    {
        SLB_ASSERT(realInput.size() >= m_fftLength, "Signal length must match FFT Size"); // TODO: zero-padding
        
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

// MARK: -------- FrequencyDomain/Helpers.hpp --------
//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved




namespace slb {
namespace AudioTraits {

namespace FrequencyDomainHelpers
{
// MARK: - Constants
constexpr int fftLength = 4096;
static_assert(Utils::isPowerOfTwo(fftLength), "FFT has to be power of 2");
constexpr int numBins = fftLength / 2 + 1;

// MARK: - Helper functions
template<typename T=float>
inline void applyHannWindow(std::vector<T>& channelSignal)
{
    int i = 0;
    for (auto& sample : channelSignal) {
        double window = 0.5 * (1 - std::cos(2*M_PI * i++ / (channelSignal.size()-1)));
        sample *= static_cast<T>(window);
    }
}

/** Create list of bins that correspond to one FrequencyRange */
static inline std::set<int> determineCorrespondingBins(const FreqBand& frequencyRange, float sampleRate)
{
    std::set<int> bins;
    
    float freqStart = std::get<0>(frequencyRange.get());
    float freqEnd = std::get<1>(frequencyRange.get());
    int expectedBinStart = static_cast<int>(std::floor(freqStart / sampleRate * fftLength));
    int expectedBinEnd = static_cast<int>(std::ceil(freqEnd / sampleRate * fftLength));
    SLB_ASSERT(expectedBinStart >= 0, "invalid frequency range");
    SLB_ASSERT(expectedBinEnd < numBins, "frequency range too high for this sampling rate");
    
    for (int i=expectedBinStart; i <= expectedBinEnd; ++i) {
        bins.insert(i);
    }
    return bins;
}

/** Create an aggregated list of bins that correspond to all in bands in the selection */
static inline std::set<int> determineCorrespondingBins(const Freqs& frequencySelection, float sampleRate)
{
    std::set<int> bins;
    
    for (const auto& frequencyRange : frequencySelection.getRanges()) {
        std::set<int> binsForThisRange = determineCorrespondingBins(frequencyRange, sampleRate);
        bins.insert(binsForThisRange.begin(), binsForThisRange.end());
    }
    return bins;
}

/** @returns the absolute values of the bin contents for a given signal, normalized to the highest-valued bin */
static inline std::vector<float> getNormalizedBinValues(std::vector<float>& channelSignal)
{
    // TODO: use overlap-add for cleaner results (?)

    RealValuedFFT fft(fftLength);
    
    // perform FFT in several chunks
    constexpr int chunkSize = fftLength;
    float numChunksFract = static_cast<float>(channelSignal.size()) / chunkSize;
    int numChunks = static_cast<int>(std::ceil(numChunksFract));
    channelSignal.resize(numChunks * chunkSize); // pad to a multiple of full chunks
    
    // Accumulated over all chunks - init with 0
    std::vector<float> accumulatedBins(numBins, 0.f);
    
    for (int chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex) {
        auto chunkBegin = channelSignal.begin() + chunkIndex * chunkSize;
        std::vector<float> chunkTimeDomain{chunkBegin, chunkBegin + chunkSize};
        
        applyHannWindow(chunkTimeDomain);
        
        std::vector<std::complex<float>> freqDomainData = fft.performForward(chunkTimeDomain);
        std::vector<float> binValuesForChunk;
        for (const auto& binValue : freqDomainData) {
            binValuesForChunk.emplace_back(std::abs(binValue));
        }
        
        // accumulate: accumulatedBins += binValues
        SLB_ASSERT(binValuesForChunk.size() == accumulatedBins.size());
        for (int k=0; k < accumulatedBins.size(); ++k) {
            accumulatedBins[k] += binValuesForChunk[k];
        }
    }
    
    // normally, we would normalize then bin values with numChunks and fftLength, but here
    // we choose to define the highest-valued bin as 0dB, therefore we normalize by it
    float maxBinValue = *std::max_element(accumulatedBins.begin(), accumulatedBins.end());
    for (auto& binValue : accumulatedBins) {
        binValue /= maxBinValue;
    }
    
    // Hard-code DC bin to 0
    accumulatedBins[0] = 0;

    return accumulatedBins;
}
} // namespace FrequencyDomainHelpers

} // namespace AudioTraits
} // namespace slb

// MARK: -------- AudioTraits-FD.hpp --------
//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved


// NOTE: DO NOT INCLUDE THIS FILE DIRECTLY; INCLUDE THIS INSTEAD: AudioTraits.hpp




namespace slb {
namespace AudioTraits {

// MARK: - Frequency Domain Audio Traits

/**
 * Evaluates if all the selected channels have frequency content in all the specified bands.
 * The spectral content outside the specified bands is not analyzed.
 *
 * To count as 'there is frequency content', it needs to be above a certain threshold in dB in at least one of the bins
 * in that band. The threshold is relative to the maximum bin value of all bins, across the entire spectrum.
 *
 * TODO: add option to re-use 'cache' the FFT results instead of recalculating every time.
 */
struct HasSignalInAllBands
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, const Freqs& frequencySelection,
                     float sampleRate, float threshold_dB = -0.5f)
    {
        if (frequencySelection.getRanges().empty()) {
            return false; // Empty frequency selection is always false
        }
        
        // Each frequency band needs to be tested individually
        for (const auto& frequencyRange : frequencySelection.getRanges()) {
            // Determine bins where signal is expected
            std::set<int> expectedBins = FrequencyDomainHelpers::determineCorrespondingBins(frequencyRange, sampleRate);
            
            for (int chNumber : selectedChannels) {
                std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
                std::vector<float> normalizedBinValues = FrequencyDomainHelpers::getNormalizedBinValues(channelSignal);
                
                bool hasValidSignalInThisRange = false;
                for (int expectedBin : expectedBins) {
                    float binValue_dB = Utils::linear2Db(normalizedBinValues.at(expectedBin));
                    if (binValue_dB >= threshold_dB) {
                        hasValidSignalInThisRange = true;
                        break; // at least one of the bins in this band has signal, skip the remaining bins in this band.
                    }
                }
                if (hasValidSignalInThisRange == false) {
                    return false; // channel did not have signal in any bin in this band
                }
            }
        }
        
        return true;
    }
};

/**
 * Evaluates if all the selected channels have frequency content in all the specified bands only, and none in the
 * rest of the spectrum. Note that the signal *can* have content in the specified bands, but does not necessarily *have
 * to* for this trait to be true.
 *
 * To count as 'there is frequency content', it needs to be above a certain threshold in dB in at least one of the bins
 * in that band. The threshold is relative to the maximum bin value of all bins, across the entire spectrum.
 *
 * If any FFT bins (that are not part of the selected frequency bands) reach the threshold, the result will be 'false'.
 *
 * TODO: add option to re-use 'cache' the FFT results instead of recalculating every time.
 */
struct HasSignalOnlyInBands
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, const Freqs& frequencySelection,
                     float sampleRate, float threshold_dB = -0.5f)
    {
        // We only need to scan 'illegal' bands for content. If these are clean, the trait is true.
        // Determine bins where signal is allowed
        std::set<int> legalBins = FrequencyDomainHelpers::determineCorrespondingBins(frequencySelection, sampleRate);
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> normalizedBinValues = FrequencyDomainHelpers::getNormalizedBinValues(channelSignal);
        
            for (int binIndex = 0; binIndex < FrequencyDomainHelpers::numBins; ++binIndex) {
                float binValue_dB = Utils::linear2Db(normalizedBinValues.at(binIndex));
                if (binValue_dB >= threshold_dB) {
                    // there's content in this bin -- is this bin 'legal' ?
                    // TODO: optimization -- change this to a vector<bool> or something (direct access rather than find)
                    if (legalBins.find(binIndex) == legalBins.end()) {
                        // this bin is not legal -> there's signal in at least one bin outside the legal bands
                        return false;
                    }
                }
            }
        }
            
        return true;
    }
};


/** Can be used as a shorthand for HasSignalOnlyInBands, where the lower limit of the band is the minimum frequency (1Hz)*/
struct HasSignalOnlyBelow
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float frequency, float sampleRate, float threshold_dB = -0.5f)
    {
        return HasSignalOnlyInBands::eval(signal, selectedChannels, Freqs{{1, frequency}}, sampleRate, threshold_dB);
    }
};

/** Can be used as a shorthand for HasSignalOnlyInBands, where the upper limit of the band is the maximum frequency (Nyquist=samplerate/2) */
struct HasSignalOnlyAbove
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float frequency, float sampleRate, float threshold_dB = -0.5f)
    {
        return HasSignalOnlyInBands::eval(signal, selectedChannels, Freqs{{frequency, sampleRate/2}}, sampleRate, threshold_dB);
    }
};

} // namespace AudioTraits
} // namespace slb


namespace slb {
namespace AudioTraits {

// MARK: - Infrastructure
template<typename F, typename ... Is>
static bool check(const ISignal& signal, const ChannelSelection& channelSelection, Is&& ... traitParams)
{
    SLB_ASSERT(signal.getNumSamples() > 0);
    std::set<int> selectedChannels = channelSelection.get();
    SLB_ASSERT(selectedChannels.size() <= signal.getNumChannels());
    std::for_each(selectedChannels.begin(), selectedChannels.end(), [&signal](auto& i) { SLB_ASSERT(i<=signal.getNumChannels()); });

    // Empty selection means all channels
    if (selectedChannels.empty()) {
        std::set<int>::iterator it = selectedChannels.end();
        for (int i=1; i <= signal.getNumChannels(); ++i) {
           it = selectedChannels.insert(it, i);
        }
    }
    
    return F::eval(signal, selectedChannels, std::forward<decltype(traitParams)>(traitParams)...);
}

/** @returns true if a >= b (taking into account tolerance [dB]) */
static inline bool areVectorsEqual(const std::vector<float>& a, const std::vector<float>& b, float tolerance_dB)
{
    SLB_ASSERT(a.size() == b.size(), "Vectors must be of equal length for comparison");
    return std::equal(a.begin(), a.end(), b.begin(), [&tolerance_dB](float v1, float v2)
    {
        float error = std::abs(Utils::linear2Db(std::abs(v1)) - Utils::linear2Db(std::abs(v2)));
        return error <= tolerance_dB;
    });
};


// MARK: - Audio Traits

/**
 * Evaluates if all of the selected channels have at least one sample above the threshold (absolute value)
 */
struct HasSignalOnAllChannels
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float threshold_dB = -96.f)
    {
        const float threshold_linear = Utils::dB2Linear(threshold_dB);
        for (int chNumber : selectedChannels) {
            auto channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            // find absolute max sample in channel signal
            auto minmax = std::minmax_element(channelSignal.begin(), channelSignal.end());
            float absmax = std::max(std::abs(*std::get<0>(minmax)), *std::get<1>(minmax));
            if (absmax < threshold_linear) {
                return false; // one channel without signal is enough to fail
            }
        }
        return true;
    }
};

/**
 * Evaluates if the signal represents a delayed version of the reference signal by a given amount of samples.
 *
 * Optionally, error tolerance can be specified for both amplitude (in dB [power]) and time (in samples).
 *
 * @note: The longer the delay, the shorter signal left to do the comparison on. Therefore, the delay time is limited
 * to a maximum of 80% of the total signal length.
 */
struct IsDelayedVersionOf
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, const ISignal& referenceSignal,
                     int delay_samples, float amplitudeTolerance_dB = 0.f, int timeTolerance_samples = 0)
    {
        SLB_ASSERT(delay_samples >= 0, "The delay must be positive");
        SLB_ASSERT(amplitudeTolerance_dB >= 0 && amplitudeTolerance_dB < 96.f, "Invalid amplitude tolerance");
        SLB_ASSERT(timeTolerance_samples >= 0 && timeTolerance_samples <= 5, "Time tolerance has to be between 0 and 5 samples");
        SLB_ASSERT((static_cast<float>(delay_samples)/signal.getNumSamples()) < .8f, "The delay cannot be longer than 80% of the signal");
        SLB_ASSERT(referenceSignal.getNumSamples() >= signal.getNumSamples() - delay_samples, "The reference signal is not long enough");

        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> channelSignalRef = referenceSignal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based

            bool thisChannelPassed = false;
            
            // Allow for some tolerance on the delay time: ±maxTimeError_samples
            // Try to match signal with all delay values in this range
            const int& error = timeTolerance_samples;
            for (int jitteredDelay = delay_samples - error; jitteredDelay <= delay_samples + error; ++jitteredDelay) {
                std::vector<float> delayedRef;
                if (jitteredDelay < 0) {
                    // negative delay: we delay the signal instead of the reference
                    delayedRef = channelSignal;
                } else {
                    delayedRef = channelSignalRef;
                }
                // delay the copy we made
                std::vector<float> zeroPadding(std::abs(jitteredDelay), 0);
                delayedRef.insert(delayedRef.begin(), zeroPadding.begin(), zeroPadding.end());
                delayedRef.resize(channelSignal.size());
                
                if (areVectorsEqual(channelSignal, delayedRef, amplitudeTolerance_dB)) {
                    thisChannelPassed = true; // We found a match for this channel
                    break;
                }
            }
            if (!thisChannelPassed) {
                return false; // none of the 'jittered' delay times was a match
            }
        }
        return true;
    }
  
};

/**
 * Evaluates if the signal has matching channels for the entire supplied selection. The matchiing is done on a
 * sample-by-sample basis.
 *
 * Optionally, error tolerance for the matching can be specified in dB
 */
struct HasIdenticalChannels
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels, float tolerance_dB = 0.f)
    {
        SLB_ASSERT(tolerance_dB >= 0 && tolerance_dB < 96.f, "Invalid amplitude tolerance");
        
        bool doAllChannelsMatch = true;
        std::vector<float> reference(0); // init with size 0
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignal = signal.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            if (reference.empty()) {
                reference = channelSignal; // Take first channel as reference
                continue; // no comparison with itself
            }
            doAllChannelsMatch = areVectorsEqual(channelSignal, reference, tolerance_dB);
        }
        
        return doAllChannelsMatch;
    }
  
};

/**
 * Evaluates if two signals have matching channels for the entire supplied selection. The matchiing is done on a
 * sample-by-sample basis.
 *
 * Optionally, error tolerance for the matching can be specified in dB
 */
struct HaveIdenticalChannels
{
    static bool eval(const ISignal& signalA, const std::set<int>& selectedChannels, const ISignal& signalB, float tolerance_dB = 0.f)
    {
        SLB_ASSERT(tolerance_dB >= 0 && tolerance_dB < 96.f, "Invalid amplitude tolerance");
        
        for (int chNumber : selectedChannels) {
            std::vector<float> channelSignalA = signalA.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            std::vector<float> channelSignalB = signalB.getChannelDataCopy(chNumber - 1); // channels are 1-based, indices 0-based
            if (!areVectorsEqual(channelSignalA, channelSignalB, tolerance_dB)) {
                return false; // one channel without a match is enough to fail
            }
        }
        return true;
    }
};

} // namespace AudioTraits
} // namespace slb
