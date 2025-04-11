#ifndef WARTHOG_UTIL_INTRIN_H
#define WARTHOG_UTIL_INTRIN_H

// intrin.h
//
// Helper functions for x86 intrinsics functions.
// Mainly support to detect when they are available to use.
//
// Header defines WARTHOG_INTRIN if intrinsics header can be used.
// Use WARTHOG_INTRIN_HAS(inst) to test if instruction set is supported.
// WARTHOG_INTRIN_HAS(BMI2) for example sees if __BMI2__ is defined.
// It must align with __##inst##__ in compiler to work.
//
// Check
// https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html

#if defined(__x86_64__) && __has_include(<immintrin.h>)
#include <immintrin.h>
#define WARTHOG_INTRIN
#define WARTHOG_INTRIN_HAS(inst) defined(__##inst##__)
#else
#define WARTHOG_INTRIN_HAS(inst) 0
#endif

#endif // WARTHOG_UTIL_X86INTRIN_H
