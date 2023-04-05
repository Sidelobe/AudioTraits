//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//  
//  © 2023 Lorenz Bucher - all rights reserved
//  https://github.com/Sidelobe/AudioTraits

#pragma once

#ifdef SLB_AMALGATED_HEADER
    #include "AudioTraits.hpp"
#else
    #include "SignalAdapters.hpp"
#endif

#include "AudioFile.h" // include https://github.com/adamstark/AudioFile

/** An adapter for AudioFile objects */
template<typename T>
class SignalAdapterAudioFile : public slb::AudioTraits::SignalAdapterStdVecVec
{
public:
    /**
     * Build a SignalAdapter from an AudioFile object.
     * @note This object holds a reference to the input data --  will not compile when given an rvalue
     */
    explicit SignalAdapterAudioFile(AudioFile<T>& audioFile) :
        SignalAdapterStdVecVec(audioFile.samples),
        m_audioFile(audioFile) {}
    
    int getNumChannels() const override { return static_cast<int>(m_audioFile.getNumChannels()); }
    int getNumSamples()  const override { return static_cast<int>(m_audioFile.getNumSamplesPerChannel()); }

private:
    const AudioFile<T>& m_audioFile;
};
