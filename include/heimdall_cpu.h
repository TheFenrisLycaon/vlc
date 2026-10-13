/*****************************************************************************
 * heimdall_cpu.h: CPU capabilities
 *****************************************************************************
 * Copyright (C) 1998-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/**
 * \file
 * This file provides CPU features detection.
 */

#ifndef HEIMDALL_CPU_H
# define HEIMDALL_CPU_H 1

#include <heimdall_threads.h>

/**
 * Retrieves CPU capability flags.
 */
HEIMDALL_API unsigned heimdall_CPU(void);

/**
 * Masks CPU capability flags.
 *
 * This overrides the cached CPU capability flags with the intersection of
 * the actually-detected CPU capabilities and the supplied mask. Bits cannot be
 * added, only cleared: masking never reports a capability that hardware
 * doesn't actually have.
 *
 * This is intended for testing purposes only (e.g. checkasm), to force
 * code paths for lesser CPU capabilities to be exercised.
 *
 * \warning This function is not thread-safe with respect to heimdall_CPU(): it
 * must be called before any thread makes its first call to heimdall_CPU(),
 * otherwise the lazy initialisation in heimdall_CPU() may race with and
 * overwrite the masked value.
 */
HEIMDALL_API void heimdall_CPU_set(unsigned mask);

/**
 * Computes CPU capability flags.
 *
 * Do not call this function directly.
 * Call heimdall_CPU() instead, which caches the correct value.
 */
unsigned heimdall_CPU_raw(void);

# if defined (__aarch64__)
#  define HAVE_FPU 1
#  define HEIMDALL_CPU_ARM_NEON 0x1
#  define HEIMDALL_CPU_ARM_SVE  0x2

#  define heimdall_CPU_ARM_NEON() ((heimdall_CPU() & HEIMDALL_CPU_ARM_NEON) != 0)
#  define heimdall_CPU_ARM_SVE()   ((heimdall_CPU() & HEIMDALL_CPU_ARM_SVE) != 0)

# elif defined (__arm__)
#  if defined (__VFP_FP__) && !defined (__SOFTFP__)
#   define HAVE_FPU 1
#  else
#   define HAVE_FPU 0
#  endif
#  define HEIMDALL_CPU_ARMv6    4
#  define HEIMDALL_CPU_ARM_NEON 2

#  if defined (__ARM_ARCH_7A__)
#   define HEIMDALL_CPU_ARM_ARCH 7
#  elif defined (__ARM_ARCH_6__) || defined (__ARM_ARCH_6T2__)
#   define HEIMDALL_CPU_ARM_ARCH 6
#  else
#   define HEIMDALL_CPU_ARM_ARCH 4
#  endif

#  define heimdall_CPU_ARMv6() ((heimdall_CPU() & HEIMDALL_CPU_ARMv6) != 0)
#  define heimdall_CPU_ARM_NEON() ((heimdall_CPU() & HEIMDALL_CPU_ARM_NEON) != 0)


# elif defined (__loongarch__)
#  define HAVE_FPU 1

# elif defined (__mips)
#  if defined (__mips_hard_float)
#   define HAVE_FPU 1
#  else
#   define HAVE_FPU 0
#  endif

# elif defined (__ppc__) || defined (__ppc64__) || defined (__powerpc__)
#  define HAVE_FPU 1
#  define HEIMDALL_CPU_ALTIVEC 2

#   define heimdall_CPU_ALTIVEC() ((heimdall_CPU() & HEIMDALL_CPU_ALTIVEC) != 0)
#  ifdef ALTIVEC
#   define HEIMDALL_ALTIVEC
#  else
#   define HEIMDALL_ALTIVEC __attribute__ ((__target__ ("altivec")))
#  endif

# elif defined (__riscv)
#  ifdef __riscv_f
#   define HAVE_FPU 1
#  else
#   define HAVE_FPU 0
#  endif
#  define HEIMDALL_CPU_RV_V 0x1
#  define HEIMDALL_CPU_RV_B 0x2

#  define heimdall_CPU_RV_V() ((heimdall_CPU() & HEIMDALL_CPU_RV_V) != 0)
#  define heimdall_CPU_RV_B() ((heimdall_CPU() & HEIMDALL_CPU_RV_B) != 0)

# elif defined (__sparc__)
#  define HAVE_FPU 1

# elif  defined (__x86_64__) || defined (__i386__)
#  define HAVE_FPU 1
#  define HEIMDALL_CPU_SSE2   0x00000080
#  define HEIMDALL_CPU_SSE3   0x00000100
#  define HEIMDALL_CPU_SSSE3  0x00000200
#  define HEIMDALL_CPU_SSE4_1 0x00000400
#  define HEIMDALL_CPU_AVX    0x00002000
#  define HEIMDALL_CPU_AVX2   0x00004000

#  if defined (__SSE__)
#   define HEIMDALL_SSE
#  else
#   define HEIMDALL_SSE __attribute__ ((__target__ ("sse")))
#  endif

#  define heimdall_CPU_SSE2() ((heimdall_CPU() & HEIMDALL_CPU_SSE2) != 0)
#  define heimdall_CPU_SSE3() ((heimdall_CPU() & HEIMDALL_CPU_SSE3) != 0)
#  define heimdall_CPU_SSSE3() ((heimdall_CPU() & HEIMDALL_CPU_SSSE3) != 0)
#  define heimdall_CPU_SSE4_1() ((heimdall_CPU() & HEIMDALL_CPU_SSE4_1) != 0)

#   define heimdall_CPU_AVX() ((heimdall_CPU() & HEIMDALL_CPU_AVX) != 0)
#  ifdef __AVX__
#   define HEIMDALL_AVX
#  else
#   define HEIMDALL_AVX __attribute__ ((__target__ ("avx")))
#  endif

#  define heimdall_CPU_AVX2() ((heimdall_CPU() & HEIMDALL_CPU_AVX2) != 0)

# else
/**
 * Are single precision floating point operations "fast"?
 * If this preprocessor constant is zero, floating point should be avoided
 * (especially relevant for audio codecs).
 */
#  define HAVE_FPU 1

# endif

/**
 * Initialises DSP functions.
 *
 * This helper looks for accelerated Digital Signal Processing functions
 * identified by the supplied type name. Those functions ares typically
 * implemented using architecture-specific assembler code with
 * Single Instruction Multiple Data (SIMD) opcodes for faster processing.
 *
 * The exact purposes and semantics of the DSP functions is uniquely identified
 * by a nul-terminated string.
 *
 * \note This function should not be used directly. It is recommended to use
 * the convenience wrapper heimdall_CPU_functions_init_once() instead.
 *
 * \param name nul-terminated type identifier (cannot be NULL)
 * \param [inout] funcs type-specific data structure to be initialised
 */
HEIMDALL_API void heimdall_CPU_functions_init(const char *name, void *restrict funcs);

# ifndef __cplusplus
/**
 * Initialises DSP functions once.
 *
 * This is a convenience wrapper for heimdall_CPU_functions_init().
 * It only initialises the functions the first time it is evaluated.
 */
static inline void heimdall_CPU_functions_init_once(const char *name,
                                               void *restrict funcs)
{
    static heimdall_once_t once = HEIMDALL_STATIC_ONCE;

    if (!heimdall_once_begin(&once)) {
        heimdall_CPU_functions_init(name, funcs);
        heimdall_once_complete(&once);
    }
}
# endif

#define set_cpu_funcs(name, activate, priority) \
    set_callback(HEIMDALL_CHECKED_TYPE(void (*)(void *), activate)) \
    set_capability(name, priority)

#endif /* !HEIMDALL_CPU_H */
