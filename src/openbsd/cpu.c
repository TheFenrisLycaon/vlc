/*****************************************************************************
 * openbsd/cpu.c: CPU detection code for OpenBSD
 *****************************************************************************
 * Copyright (C) 2012 Rémi Denis-Courmont
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/auxv.h>

#include <heimdall_common.h>
#include <heimdall_cpu.h>

#if defined (__aarch64__)
unsigned heimdall_CPU_raw(void)
{
    unsigned int flags = 0;
    unsigned long hwcap = 0;
    //unsigned long hwcap2 = 0; // TODO: SVE2

    elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));
    // elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap2)); // TODO: SVE2

    /* HWCAP_FP (HAVE_FPU) is statically assumed. */
    if (hwcap & HWCAP_ASIMD)
        flags |= HEIMDALL_CPU_ARM_NEON;
    if (hwcap & HWCAP_SVE)
        flags |= HEIMDALL_CPU_ARM_SVE;

    return flags;
}

#elif defined (__arm__)
unsigned heimdall_CPU_raw(void)
{
    unsigned int flags = 0;
    unsigned long hwcap = 0;

    elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));

    /* TLS implies ARMv6, Thumb-EE and VFP imply ARMv7 */
    if (hwcap & (HWCAP_TLS|HWCAP_THUMBEE|HWCAP_VFP))
        flags |= HEIMDALL_CPU_ARMv6; /* SIMD */
    if (hwcap & HWCAP_NEON)
        flags |= HEIMDALL_CPU_ARM_NEON; /* Advanced SIMD */

    return flags;
}

#elif defined (__powerpc__) /* both 32- and 64-bit */
unsigned heimdall_CPU_raw(void)
{
    unsigned int flags = 0;
    unsigned long hwcap = 0;

    elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));

    if (hwcap & PPC_FEATURE_HAS_ALTIVEC)
        flags |= HEIMDALL_CPU_ALTIVEC;

    return flags;
}

#elif defined (__riscv)
# define HWCAP_RV(letter) (1LU << ((letter) - 'A'))

unsigned heimdall_CPU_raw(void)
{
    unsigned int flags = 0;
    unsigned long hwcap = 0;

    elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));

    if (hwcap & HWCAP_RV('B'))
        flags |= HEIMDALL_CPU_RV_B;
    if (hwcap & HWCAP_RV('V'))
        flags |= HEIMDALL_CPU_RV_V;

    return flags;
}
#endif
