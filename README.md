```
  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
```

### An extensible C++ testing abstraction for audio signals

![](https://img.shields.io/github/license/Sidelobe/AudioTraits)
![](https://img.shields.io/badge/C++14-header--only-blue.svg?style=flat&logo=c%2B%2B)
![](https://img.shields.io/badge/dependencies-STL_only-blue)

*AudioTraits* is an abstraction designed to make testing of audio processing more convenient and readable. An 'Audio Trait' analyzes a given audio signal (some or all of its channels) and checks for a certain property. The result of this check is boolean, which allows this to be easily integrated in any unit test framework.

*AudioTraits* is easily extensible - users can define their own traits and signal adapters.

In the testing landscape, *AudioTraits* is meant to go somewhere between basic instantiation-only and fully-fledged numerical precision tests (e.g. comparison against reference data)

- **What it was designed for**: testing the logic and functionality of audio blocks with idiomatic code
- **What it was NOT designed for**: precision, performance, efficiency, use in production code

### Usage Example
```cpp
#include "AudioTraits.hpp"
using namespace slb;
using namespace AudioTraits; // for convenience

// Assume buffer contains some data with dimensions [channels=6][samples=16] 
std::vector<std::vector<float>> buffer = { ... };
SignalAdapterStdVecVec signal(buffer);

// Check if there is signal present (time domain)
REQUIRE(check<HasSignalOnAllChannels>(signal, {})); // signal on all channels 
REQUIRE(check<HasSignalOnAllChannels>(signal, {1,2})); // signal on channels 1 and 2
REQUIRE(check<HasSignalOnAllChannels>(signal, {1, 2, {4,6}}));  // signal on channels 1, 2 and 4-6
REQUIRE(check<HasSignalOnAllChannels>(signal, {5}, -40.f)); // signal on chan 5 is above -40dB

// Check if a signal is a delayed version of another 
// (tolerance for delay and amplitude can be adjusted)
SignalAdapterStdVecVec delayedSignal = ...; // assume this is 'signal' delayed by 4 samples
REQUIRE(check<IsDelayedVersionOf>(signal, {}, delayedSignal, 4));
REQUIRE_FALSE(check<IsDelayedVersionOf>(signal, {}, delayedSignal, 2));

// Frequency-Domain Traits:
constexpr float sampleRate = 48000;

// signal on chan 1 has content at around 1000Hz
REQUIRE(check<HasSignalInAllBands>(signal, {1}, Freqs{1000}, sampleRate));
// signal has content in the band 500Hz-1000Hz in all channels
REQUIRE(check<HasSignalInAllBands>(signal, {}, Freqs{{500, 1000}}, sampleRate));
// signal has content in 100Hz-200Hz that is -30dB below the maximum over the entire spectrum
REQUIRE(check<HasSignalInAllBands>(signal, {}, Freqs{{100, 200}}, sampleRate, -30.f));

// signal only has content above -5dB in the band 20-5000 Hz (relative to the spectral maximum)
REQUIRE(check<HasSignalOnlyInBands>(signal, {}, Freqs{{20, 5000}}, sampleRate, -5.f));
// signal only has content below 4kHz in all channels
REQUIRE(check<HasSignalOnlyBelow>(signal, {}, 4000, sampleRate));

```

>NOTE: For the frequency-domain traits, a 4096-point FFT is calculated for every single check. This is - needless to say - very inefficient, but a conscious design choice for the sake of simplicity. Adding more optimized, stateful 'traits' is quite easy, should there be a need.

### Extension: Custom Traits
Defining custom traits is very straightforward: a traits is simply a functor with a static (stateless) function that returns a boolean:

```cpp
#include "AudioTraits.hpp"
struct HasOddNumberOfSamples
{
    static bool eval(const ISignal& signal, const std::set<int>& selectedChannels)
    {
        if (signal.getNumSamples() % 2 == 0) {
            return false; // number of samples is even
        } else {
            return true; // number of samples is odd
        }
    }
};
```
- The number of parameters of the `eval()` is variable, so a custom trait may add any number of additional parameters besides `signal` and `selectedChannels`.

- `ISignal` is the common interface for all signal to be analyzed. A signal is a minimalistic 2-dimensional construct with a number of channels and samples.

- Signal 'adapters' are pre-defined for raw pointers (e.g. `float**`) and `std::vector<std::vector<T>>`, and additional adapters can easily be added for other audio signal sources.

### Requirements / Compatibility

 - C++14, STL only
 - Compiled & Tested with:
 	- Linux / macos / Windwos
 	- GCC, Clang and MSVC
 	- `x86_64` and `arm64` architectures
	
### Build Status / Quality Metrics

![](https://img.shields.io/badge/branch-main-blue)
[![Build & Test](https://github.com/Sidelobe/AudioTraits/actions/workflows/workflow.yml/badge.svg?branch=main)](https://github.com/Sidelobe/AudioTraits/actions/workflows/workflow.yml)
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
![](https://img.shields.io/badge/branch-develop-blue)
[![Build & Test](https://github.com/Sidelobe/AudioTraits/actions/workflows/workflow.yml/badge.svg?branch=develop)](https://github.com/Sidelobe/AudioTraits/actions/workflows/workflow.yml)

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=Sidelobe_AudioTraits)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=coverage)](https://sonarcloud.io/summary/new_code?id=Sidelobe_AudioTraits)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=bugs)](https://sonarcloud.io/summary/new_code?id=Sidelobe_AudioTraits)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=Sidelobe_AudioTraits)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=Sidelobe_AudioTraits)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=Sidelobe_AudioTraits)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=Sidelobe_AudioTraits)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=security_rating)](https://sonarcloud.io/dashboard?id=Sidelobe_AudioTraits)
[![Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=Sidelobe_AudioTraits&metric=sqale_index)](https://sonarcloud.io/dashboard?id=Sidelobe_AudioTraits)
