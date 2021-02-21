//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include "SignalAdapters.hpp"

#include "AudioFile.h" // include https://github.com/adamstark/AudioFile


/** An adapter for AudioFile objects */
template<typename T>
class SignalAdapterAudioFile : public slb::AudioTraits::ISignal
{
public:
    /**
     * Build a SignalAdapter from an AudioFile object.
     * @note This object holds a reference to the input data --  will not compile when given an rvalue
     */
    explicit SignalAdapterAudioFile(AudioFile<T>& audioFile) :
        m_audioFile(audioFile),
        m_channelPointers(audioFile.getNumChannels())
    {
        for (int i = 0; i < static_cast<int>(m_audioFile.getNumChannels()); ++i) {
            m_channelPointers[i] = m_audioFile.samples[i].data();
        }
    }
    
    int getNumChannels() const override { return static_cast<int>(m_audioFile.getNumChannels()); }
    int getNumSamples()  const override { return static_cast<int>(m_audioFile.getNumSamplesPerChannel());  }
    const float* const* getData()    const override { return m_channelPointers.data();  }

private:
    const AudioFile<T>& m_audioFile;
    std::vector<const float*> m_channelPointers;
};
