//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//  
//  © 2023 Lorenz Bucher - all rights reserved
//  https://github.com/Sidelobe/AudioTraits

#pragma once

#include <catch2/catch.hpp>

/* Macro to detect if exceptions are disabled (works on GCC, Clang and MSVC) 3 */
#ifndef __has_feature
    #define __has_feature(x) 0
#endif
#if !__has_feature(cxx_exceptions) && !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
  #define SLB_EXCEPTIONS_DISABLED
#endif

// When exceptions are disabled, we redefine catch2's REQUIRE_THROWS, so we can compile.
// Any REQUIRE_THROWS statements in tests will dissappear / do nothing
#ifdef SLB_EXCEPTIONS_DISABLED
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
