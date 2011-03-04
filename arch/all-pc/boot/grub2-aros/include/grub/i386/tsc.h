/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KERNEL_CPU_TSC_HEADER
#define KERNEL_CPU_TSC_HEADER   1

#include <grub/types.h>

/* Read the TSC value, which increments with each CPU clock cycle. */
static __inline grub_uint64_t
grub_get_tsc (void)
{
  grub_uint32_t lo, hi;

  /* The CPUID instruction is a 'serializing' instruction, and
     avoids out-of-order execution of the RDTSC instruction. */
#ifdef APPLE_CC
  __asm__ __volatile__ ("xorl %%eax, %%eax\n\t"
#ifdef __x86_64__
			"push %%rbx\n"
#else
			"push %%ebx\n"
#endif
			"cpuid\n"
#ifdef __x86_64__
			"pop %%rbx\n"
#else
			"pop %%ebx\n"
#endif
			:::"%rax", "%rcx", "%rdx");
#else
  __asm__ __volatile__ ("xorl %%eax, %%eax\n\t"
			"cpuid":::"%rax", "%rbx", "%rcx", "%rdx");
#endif
  /* Read TSC value.  We cannot use "=A", since this would use
     %rax on x86_64. */
  __asm__ __volatile__ ("rdtsc":"=a" (lo), "=d" (hi));

  return (((grub_uint64_t) hi) << 32) | lo;
}

#ifdef __x86_64__

static __inline int
grub_cpu_is_cpuid_supported (void)
{
  grub_uint64_t id_supported;

  __asm__ ("pushfq\n\t"
           "popq %%rax             /* Get EFLAGS into EAX */\n\t"
           "movq %%rax, %%rcx      /* Save original flags in ECX */\n\t"
           "xorq $0x200000, %%rax  /* Flip ID bit in EFLAGS */\n\t"
           "pushq %%rax            /* Store modified EFLAGS on stack */\n\t"
           "popfq                  /* Replace current EFLAGS */\n\t"
           "pushfq                 /* Read back the EFLAGS */\n\t"
           "popq %%rax             /* Get EFLAGS into EAX */\n\t"
           "xorq %%rcx, %%rax      /* Check if flag could be modified */\n\t"
           : "=a" (id_supported)
           : /* No inputs.  */
           : /* Clobbered:  */ "%rcx");

  return id_supported != 0;
}

#else

static __inline int
grub_cpu_is_cpuid_supported (void)
{
  grub_uint32_t id_supported;

  __asm__ ("pushfl\n\t"
           "popl %%eax             /* Get EFLAGS into EAX */\n\t"
           "movl %%eax, %%ecx      /* Save original flags in ECX */\n\t"
           "xorl $0x200000, %%eax  /* Flip ID bit in EFLAGS */\n\t"
           "pushl %%eax            /* Store modified EFLAGS on stack */\n\t"
           "popfl                  /* Replace current EFLAGS */\n\t"
           "pushfl                 /* Read back the EFLAGS */\n\t"
           "popl %%eax             /* Get EFLAGS into EAX */\n\t"
           "xorl %%ecx, %%eax      /* Check if flag could be modified */\n\t"
           : "=a" (id_supported)
           : /* No inputs.  */
           : /* Clobbered:  */ "%rcx");

  return id_supported != 0;
}

#endif

static __inline int
grub_cpu_is_tsc_supported (void)
{
  if (! grub_cpu_is_cpuid_supported ())
    return 0;

  grub_uint32_t features;
#ifdef APPLE_CC
  __asm__ ("movl $1, %%eax\n\t"
#ifdef __x86_64__
	   "push %%rbx\n"
#else
	   "push %%ebx\n"
#endif
	   "cpuid\n"
#ifdef __x86_64__
	   "pop %%rbx\n"
#else
	   "pop %%ebx\n"
#endif
           : "=d" (features)
           : /* No inputs.  */
	   : /* Clobbered:  */ "%rax", "%rcx");
#else
  __asm__ ("movl $1, %%eax\n\t"
           "cpuid\n"
           : "=d" (features)
           : /* No inputs.  */
           : /* Clobbered:  */ "%rax", "%rbx", "%rcx");
#endif
  return (features & (1 << 4)) != 0;
}

void grub_tsc_init (void);
grub_uint64_t grub_tsc_get_time_ms (void);

#endif /* ! KERNEL_CPU_TSC_HEADER */
