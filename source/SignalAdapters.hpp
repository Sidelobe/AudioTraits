//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <vector>

#include "ChannelSelection.hpp"
#include "Utils.hpp"

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
    
    /** @returns a non-modifiable reference to the data of the given channelIndex (0-based) */
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
    SignalAdapterRaw(const float* const* rawSignal, int numChannels, int numSamples) :
    m_numChannels(numChannels),
    m_numSamples(numSamples),
    m_signal(rawSignal) {}

    int getNumChannels() const override { return m_numChannels; }
    int getNumSamples()  const override { return m_numSamples; }
    const float* const* getData() const override { return m_signal; }
    std::vector<float> getChannelDataCopy(int channelIndex) const override
    {
        ASSERT(channelIndex < m_numChannels);
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
    SignalAdapterStdVecVec(std::vector<std::vector<float>>& vector2D) :
        m_vector2D(vector2D),
        m_channelPointers(vector2D.size())
    {
        for (int i = 0; i < static_cast<int>(vector2D.size()); ++i) {
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
