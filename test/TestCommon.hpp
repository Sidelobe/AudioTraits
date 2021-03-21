//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <catch2/catch.hpp>

/* Macro to detect if exceptions are disabled (works on GCC, Clang and MSVC) 3 */
#ifndef __has_feature
    #define __has_feature(x) 0
#endif
#if !__has_feature(cxx_exceptions) && !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
  #define EXCEPTIONS_DISABLED
#endif

// When exceptions are disbled, we redefine catch2's REQUIRE_THROWS, so we can compile.
// Any REQUIRE_THROWS statements in tests will dissappear / do nothing
#ifdef EXCEPTIONS_DISABLED
    #define REQUIRE_THROWS_CATCH2 REQUIRE_THROWS
    #undef REQUIRE_THROWS
    #define REQUIRE_THROWS(...)
    #define REQUIRE_NOTHROW_CATCH2 REQUIRE_NOTHROW
    #undef REQUIRE_NOTHROW
    #define REQUIRE_NOTHROW(...)
#endif

// classic preprocessor hack to stringify -- double expansion is required
// https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace TestCommon
{
using stl_size_type = typename std::vector<float>::size_type;
constexpr stl_size_type STL(int i) { return static_cast<stl_size_type>(i); }

static inline std::vector<float> createRandomVector(int length, int seed=0)
{
    std::vector<float> result(STL(length));
    std::mt19937 engine(static_cast<unsigned>(seed));
    std::uniform_real_distribution<> dist(-1, 1); //(inclusive, inclusive)
    for (auto& sample : result) {
        sample = static_cast<float>(dist(engine));
    }
    return result;
}

static inline std::vector<int> createRandomVectorInt(int length, int seed=0)
{
    std::vector<int> result(STL(length));
    std::mt19937 engine(static_cast<unsigned>(seed));
    std::uniform_real_distribution<> dist(-1, 1); //(inclusive, inclusive)
    for (auto& sample : result) {
        sample = static_cast<int>(1000*dist(engine));
    }
    return result;
}

template<typename T>
static std::vector<T> createDirac(int lengthSamples)
{
    std::vector<T> result(lengthSamples);
    result[0] = static_cast<T>(1.0);
    std::fill(result.begin()+1, result.end(), static_cast<T>(0.0));
    return result;
}

template<typename T>
static std::vector<T> createSine(float frequency, float fs, int lengthSamples)
{
    std::vector<T> result(lengthSamples);
    double angularFrequency = 2 * M_PI * frequency / fs;
    double phase = angularFrequency;
    std::for_each(result.begin(), result.end(), [&phase, &angularFrequency](auto& value)
    {
        phase = fmod(phase + angularFrequency, 2.f * static_cast<float>(M_PI));
        value = static_cast<T>(std::sin(phase));
    });
    return result;
}

static inline std::string resolveTestFile(const std::string& path)
{
    // If source directory was defined during compilation use that instead of current working directory
    #ifdef SOURCE_DIR
        return std::string(TOSTRING(SOURCE_DIR)) + "/test/test_data/" + path;
    #else
        assert("SOURCE_DIR not defined!")
    #endif

}

/**
 * Returns the size of a static C array in number of elements. Also works for multidimensional arrays.
 */
template<class T> constexpr int getRawArrayLength(const T& a)
{
    return sizeof(a) / sizeof(typename std::remove_all_extents<T>::type);
}


} // namespace TestCommon
