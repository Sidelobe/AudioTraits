//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <type_traits>

#define UNUSED(x) (void)x

/* Macro to detect if exceptions are disabled (works on GCC, Clang and MSVC) 3 */
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#if !__has_feature(cxx_exceptions) && !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
  #define EXCEPTIONS_DISABLED
#endif

namespace slb
{

// MARK: - Assertion handling
namespace Assertions
{
#ifndef ASSERT
    #define ASSERT(condition, ...) Assertions::handleAssert(#condition, condition, __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef ASSERT_ALWAYS
    #define ASSERT_ALWAYS(...) Assertions::handleAssert("", false, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/**
 * Custom assertion handler
 *
 * @note: this assertion handler is constexpr - to allow its use inside constexpr functions.
 * The handler will still be evaluated at runtime, but memory is only allocated IF the assertion is triggered.
 */
static constexpr void handleAssert(const char* conditionAsText, bool condition, const char* file, int line, const char* message = "")
{
    if (condition == true) {
        return;
    }
    
#ifdef EXCEPTIONS_DISABLED
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
    return powf(10.f, (value_dB/20.f));
}

static inline float linear2Db(float value_linear)
{
    return 20.f * log10f(value_linear);
}

} // namespace Utils


} // namespace slb
