//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <vector>

#include "ChannelSelection.hpp"

namespace slb {
namespace AudioTraits {

// MARK: - Infrastructure

/**
 * Signal Interface - wraps around an existing signal of arbitrary type.
 *
 * Guarantees: will no modify the underlying signal, only analyze it
 */
class ISignal
{
public:
    virtual ~ISignal() = default;
    virtual int getNumChannels() const = 0;
    virtual int getNumSamples() const = 0;
    virtual const float* const* getData() const = 0; // TODO: make const float const* const* or does this only make sense in ctor ???
};


class SignalAdapterRaw : public ISignal
{
public:
    SignalAdapterRaw(const float* const* rawSignal, int numChannels, int numSamples) :
    m_numChannels(numChannels),
    m_numSamples(numSamples),
    m_signal(rawSignal) {}
    
    /** convenience ctor */
    SignalAdapterRaw(const float* const* rawSignal, std::size_t numChannels, std::size_t numSamples)
        : SignalAdapterRaw(rawSignal, static_cast<int>(numChannels), static_cast<int>(numSamples))
    {}

    int getNumChannels() const override { return m_numChannels; }
    int getNumSamples()  const override { return m_numSamples;  }
    const float* const* getData()    const override { return m_signal;  }

private:
    const int m_numChannels;
    const int m_numSamples;
    const float* const* m_signal;
};

class SignalAdapter2DStdVector : public ISignal
{
public:
    SignalAdapter2DStdVector(std::vector<std::vector<float>>& vector2D) :
        m_vector2D(vector2D),
        m_channelPointers(vector2D.size())
    {
        for (int i = 0; i < static_cast<int>(vector2D.size()); ++i) {
            m_channelPointers[i] = m_vector2D[i].data();
        }
    }
    
    int getNumChannels() const override { return static_cast<int>(m_vector2D.size()); }
    int getNumSamples()  const override { return static_cast<int>(m_vector2D.at(0).size());  }
    const float* const* getData()    const override { return m_channelPointers.data();  }

private:
    const std::vector<std::vector<float>>& m_vector2D;
    std::vector<const float*> m_channelPointers;
};

} // namespace AudioTraits
} // namespace slb
