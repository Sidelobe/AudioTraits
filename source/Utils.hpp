//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <sstream>
#include <type_traits>

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
